#include "config.h"
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <regex>
#include <string>
#include <vector>
using namespace std;

enum Ops {
  LW,     // 0
  FLW,    // 1
  SW,     // 2
  FSW,    // 3
  ADD,    // 4
  SUB,    // 5
  BEQ,    // 6
  BNE,    // 7
  FADD,   // 8
  FSUB,   // 9
  FMUL,   // 10
  FDIV,   // 11
  NULL_OP // place holder op
};

class Instruction {
public:                        // the registers will just be ints of the register number (rd=6 means rd=f6 or rd=x6 (depends on op))
  Ops op;                      // op type
  int rd;                      // destination register
  int rs1;                     // source register 1
  int rs2;                     // source register 2
  int address;                 // for loads and stores
  int latency;                 // how long instruction takes to complete
  bool waiting_on_rs1;         // is rs1 ready
  bool waiting_on_rs2;         // is rs2 ready
  int issue_cycle = -1;        // when instruction was issued
  int execute_start = -1;      // when execute start
  int execute_end = -1;        // when execute end
  int memory_read = -1;        // when mem read
  int write_results = -1;      // when wrote result
  int commits = -1;            // when committed
  string original_instruction; // original instruction string for printing
  Instruction() {
  }
  Instruction(Ops op_, int rd_, int rs1_, int rs2_, int addr, int lat, bool wrs1, bool wrs2, string oi) {
    op = op_;
    rd = rd_;
    rs1 = rs1_;
    rs2 = rs2_;
    address = addr;
    latency = lat;
    waiting_on_rs1 = wrs1;
    waiting_on_rs2 = wrs2;
    original_instruction = oi;
  }
};

// RAT element that says if the source register is waiting on a rob entry to write to cbd
class RegisterAliasTableEntry {
public:
  bool rob_point; // is it currently point to a rob
  int rob_index;  // if so where (-1 means no)
  int value_ready_cycle;
  RegisterAliasTableEntry(bool rp, int ri) {
    rob_point = rp;
    rob_index = ri;
    value_ready_cycle = -1;
  }
};

/*
What the RAT does:
when an instruction is getting source operands it goes to the rat
if the value is in the rat that means its being calculated and it
points to what rob entry (via the rob index number) will produce the value
*/

class RATs {
public:
  vector<RegisterAliasTableEntry> f_rat; // fp instructions
  vector<RegisterAliasTableEntry> i_rat; // int instructions
  RATs() {
    for (int i = 0; i < 32; i++) {
      f_rat.push_back(RegisterAliasTableEntry(false, -1));
      i_rat.push_back(RegisterAliasTableEntry(false, -1));
    }
  }
};

// just need to keep track of reservation station usage so this will be made a global
// variable to keep track for each stage
class ReservationStationUsage {
public:
  int eff_addr_in_use = 0;
  int fp_add_in_use = 0;
  int fp_mul_in_use = 0;
  int int_in_use = 0;
  int rob_in_use = 0;
  ReservationStationUsage() {
  }
};

class ROB_Entry {
public:
  Instruction inst;     // instruction
  Ops op;               // the enum op type
  int rob_id;           // rob id for tracking where the rat is pointing
  int time_left;        // latency tracker
  int cycle_entered;    // when it entered the rob, used for finding head
  bool wb_done;         // write back stage done
  bool committed;       // committed stage done
  bool execute_started; // ex stage started
  bool executed;        // finished the ex stage
  bool need_mem;        // if the op needs the memory read stage
  bool need_cdb;        // if it needs to write to the common data bus
  bool all_done;        // this rob entry is all done
  int wb_cycle;         // prevent from executing same cycle as a the rs was written
  ROB_Entry(Instruction i, Ops o, int rid) {
    inst = i;
    op = o;
    time_left = i.latency;
    rob_id = rid;
    cycle_entered = -1; // will be -1 when empty slot for now
    wb_done = false;
    execute_started = false;
    executed = false;
    committed = false;
    need_mem = false;
    need_cdb = true; // more need this than not so default to true
    all_done = false;
    wb_cycle = -1;
  }
};

// FUNCTIONS
void dynam_schedule(Config config);                 // the scheduling function
queue<Instruction> get_instructions(Config config); // instruction getter
int register_parser(string reg);                    // parse instruction for registers
void set_reqs_in_ROB(Ops op, int rob_index);        // set if the instruction needs mem or doesnt need the cbd
void issue(Config config, queue<Instruction> &instructions, RATs &RATs, ReservationStationUsage &res_station_use);
pair<vector<Ops>, bool> execute(Config config, RATs &RATs);
bool mem(Config config, RATs &RATs);
void write_result(Config config, RATs &RATs);
pair<int, Ops> commit(Config config, RATs &RATs);
bool check_if_rob_entry_wrote_rs_yet(int rob_id, RATs &RATs, bool is_rs1); // checks if register for an instruction is ready
void print_instruction_cycles(ROB_Entry rob_entry);
bool check_store_addresses(int address, int issue_cycle); // make sure no data true dependencies are happening
bool is_older_store(int issue_cycle);                     // determine if an older store exists
void true_data_dependency_load_counter();
void data_mem_delay_counter(bool was_store_committed);
bool check_if_load_is_waiting_on_store(int issue_cycle, int address);

// GLOBAL VARIABLES
int cycle = 1;
stack<int> ROB_INDEX_STACK;
vector<ROB_Entry> ROB;
RATs rats = RATs();
int ROB_delays = 0;
int res_station_delays = 0;
int data_mem_conflict_delays = 0;
int true_data_dependence_delays = 0;
int miscounted_delays = 0;
ReservationStationUsage res_stations = ReservationStationUsage();
// i dont normally use globals but man this makes some stuff easy

// masive main function
int main() {
  Config config = Config();
  config.print_config();
  for (int i = config.reorder_buffer; i > 0; i--) {
    ROB_INDEX_STACK.push(i);
  }
  dynam_schedule(config);
}

// reads instructions line by line from stdin, stores them in a queue
queue<Instruction> get_instructions(Config config) {
  string instruction;
  queue<Instruction> instruction_list;
  string rd;
  string rs1;
  string rs2;
  while (getline(cin, instruction)) {
    if (instruction.empty()) {
      continue;
    }
    regex delimiters("[ ,():\\t]+"); // regex parsing delimiters
    vector<string> pi = {};          // pi = parsed instructions, but also not all the way parsed
    sregex_token_iterator it(instruction.begin(), instruction.end(), delimiters, -1);
    sregex_token_iterator end;
    while (it != end) {
      if (!it->str().empty()) {
        pi.push_back(it->str());
      }
      ++it;
    }
    // ARITHMETIC INSTRUCTION PARSED VECTOR FORMAT: [op, rd, rs1, rs2]
    // LOAD INSTRUCTION PARSED VECTOR FORMAT:       [op, rd, 34, rs1, address] (34 doesnt matter, just a memory immediate offset so no hazards)
    // STORE INSTRUCTION PARSED VECTOR FORMAT:      [op, rs1, 37, rs2, address] (37 doesnt matter, just a memory immediate offset so no hazards)
    // BRANCH INSTRUCTION PARSED VECTOR FORMAT:     [op, rs1, rs2, address]

    // map of  instruction strings to corresponding enum type
    map<string, Ops> op_map_translator = {
        {"lw", LW},
        {"flw", FLW},
        {"sw", SW},
        {"fsw", FSW},
        {"add", ADD},
        {"sub", SUB},
        {"beq", BEQ},
        {"bne", BNE},
        {"fadd.s", FADD},
        {"fsub.s", FSUB},
        {"fmul.s", FMUL},
        {"fdiv.s", FDIV}};
    Ops op = op_map_translator[pi[0]];
    int rd = -1;
    int rs1 = -1;
    int rs2 = -1;
    int address = -1;
    int latency = 1;
    // depending on op type, parse in different ways
    switch (op) {
    case LW:
    case FLW:
      rd = register_parser(pi[1]);
      rs1 = register_parser(pi[3]);
      address = stoi(pi[4]);
      break;
    case SW:
    case FSW:
      rs1 = register_parser(pi[1]);
      rs2 = register_parser(pi[3]);
      address = stoi(pi[4]);
      break;
    case ADD:
    case SUB:
    case FADD:
    case FSUB:
    case FMUL:
    case FDIV:
      rd = register_parser(pi[1]);
      rs1 = register_parser(pi[2]);
      rs2 = register_parser(pi[3]);
      switch (op) {
      case FADD:
        latency = config.fp_add_latency;
        break;
      case FSUB:
        latency = config.fp_sub_latency;
        break;
      case FMUL:
        latency = config.fp_mul_latency;
        break;
      case FDIV:
        latency = config.fp_div_latency;
        break;
      default:
        latency = 1;
        break;
      }
      break;
    case BEQ:
    case BNE:
      rs1 = register_parser(pi[1]);
      rs2 = register_parser(pi[2]);
      break;
    default:
      break;
    }

    instruction_list.push(Instruction(op, rd, rs1, rs2, address, latency, false, false, instruction));
  }

  return instruction_list;
}

void dynam_schedule(Config config) {
  cout << "                    Pipeline Simulation\n";
  cout << "-----------------------------------------------------------\n";
  cout << "                                      Memory Writes\n";
  cout << "     Instruction      Issues Executes  Read  Result Commits\n";
  cout << "--------------------- ------ -------- ------ ------ -------\n";
  queue<Instruction> instructions = get_instructions(config);
  pair<int, Ops> commit_val;               // checks if commit op was a store and what address was stored
  pair<vector<Ops>, bool> op_execute_done; // checks if store execution finished for res station freeing

  // it seems backwards but you want to do the stages in reverse order to prevent an instruction from doing multiple
  // stages in one cycle
  while (!instructions.empty() || !ROB.empty()) {
    commit_val = {-1, NULL_OP};

    op_execute_done = {{}, false};
    // commit
    commit_val = commit(config, rats);
    // write
    write_result(config, rats);
    // if commit was not a store then we can do the mem read stage
    if (commit_val.second != FSW && commit_val.second != SW) {
      mem(config, rats);
    } else {
      data_mem_delay_counter(true);
    }
    // execute
    op_execute_done = execute(config, rats);
    // issue next instruction
    issue(config, instructions, rats, res_stations);
    // branch and store instructions leave the res stations at different times than other
    // instruction types so it needs to be evicted here
    if (op_execute_done.second) {
      for (auto i : op_execute_done.first) {
        if (i == FSW || i == SW) {
          res_stations.eff_addr_in_use -= 1;
        } else if (i == BEQ || i == BNE) {
          res_stations.int_in_use -= 1;
        }
      }
    }
    cycle += 1;
    if (cycle == 10000) {
      cout << "bruh\n";
      exit(1);
    }
  }
  cout << "\n\n";
  cout << "Delays\n";
  cout << "------\n";
  cout << "reorder buffer delays: " << ROB_delays << "\n";
  cout << "reservation station delays: " << res_station_delays << "\n";
  cout << "data memory conflict delays: " << data_mem_conflict_delays /*- miscounted_delays*/ << "\n";
  cout << "true dependence delays: " << true_data_dependence_delays << "\n";
}
// regex instruction parsing
int register_parser(string reg) {
  if (reg.empty()) return -1;
  return stoi(reg.substr(1));
}
// issue out the instruction, make sure there is room in both rob and reservation station.
// check if rs1 or rs2 are currently in the ROB and if so mark them as not ready
void issue(Config config, queue<Instruction> &instructions, RATs &RATs, ReservationStationUsage &res_station_use) {
  if (res_station_use.rob_in_use == config.reorder_buffer || instructions.empty()) {
    if (!instructions.empty()) {
      ROB_delays += 1;
    }
    return;
  }
  Instruction inst = instructions.front();
  int ROB_index;
  switch (inst.op) {
  case ADD:
  case SUB:
    if (config.ints_buffer > res_station_use.int_in_use) {
      if (RATs.i_rat[inst.rs1].rob_point) { // source register 1 is pointing to ROB entry
        inst.waiting_on_rs1 = true;
        inst.rs1 = RATs.i_rat[inst.rs1].rob_index;
      } else {
        inst.waiting_on_rs1 = false;
      }
      if (RATs.i_rat[inst.rs2].rob_point) { // source register 2 is pointing to ROB entry
        inst.waiting_on_rs2 = true;
        inst.rs2 = RATs.i_rat[inst.rs2].rob_index;
      } else {
        inst.waiting_on_rs2 = false;
      }
      ROB_index = ROB_INDEX_STACK.top();
      ROB_INDEX_STACK.pop();
      RATs.i_rat[inst.rd].rob_index = ROB_index;
      RATs.i_rat[inst.rd].rob_point = true;
      inst.issue_cycle = cycle;
      ROB.push_back(ROB_Entry(inst, inst.op, ROB_index));
      res_station_use.rob_in_use += 1;
      res_station_use.int_in_use += 1;

      instructions.pop();
    } else {
      res_station_delays += 1;
    }
    break;
  case FADD:
  case FSUB:
    if (config.fp_adds_buffer > res_station_use.fp_add_in_use) {
      if (RATs.f_rat[inst.rs1].rob_point) { // source register 1 is pointing to ROB entry
        inst.waiting_on_rs1 = true;
        inst.rs1 = RATs.f_rat[inst.rs1].rob_index;
      } else {
        inst.waiting_on_rs1 = false;
      }
      if (RATs.f_rat[inst.rs2].rob_point) { // source register 2 is pointing to ROB entry
        inst.waiting_on_rs2 = true;
        inst.rs2 = RATs.f_rat[inst.rs2].rob_index;
      } else {
        inst.waiting_on_rs2 = false;
      }
      ROB_index = ROB_INDEX_STACK.top();
      ROB_INDEX_STACK.pop();
      RATs.f_rat[inst.rd].rob_index = ROB_index;
      RATs.f_rat[inst.rd].rob_point = true;
      inst.issue_cycle = cycle;
      ROB.push_back(ROB_Entry(inst, inst.op, ROB_index));
      res_station_use.rob_in_use += 1;
      res_station_use.fp_add_in_use += 1;
      instructions.pop();
    } else {
      res_station_delays += 1;
    }
    break;
  case FMUL:
  case FDIV:
    if (config.fp_muls_buffer > res_station_use.fp_mul_in_use) {
      if (RATs.f_rat[inst.rs1].rob_point) { // source register 1 is pointing to ROB entry
        inst.waiting_on_rs1 = true;
        inst.rs1 = RATs.f_rat[inst.rs1].rob_index;
      } else {
        inst.waiting_on_rs1 = false;
      }
      if (RATs.f_rat[inst.rs2].rob_point) { // source register 2 is pointing to ROB entry
        inst.waiting_on_rs2 = true;
        inst.rs2 = RATs.f_rat[inst.rs2].rob_index;
      } else {
        inst.waiting_on_rs2 = false;
      }
      ROB_index = ROB_INDEX_STACK.top();
      ROB_INDEX_STACK.pop();
      RATs.f_rat[inst.rd].rob_index = ROB_index;
      RATs.f_rat[inst.rd].rob_point = true;
      inst.issue_cycle = cycle;
      ROB.push_back(ROB_Entry(inst, inst.op, ROB_index));
      res_station_use.rob_in_use += 1;
      res_station_use.fp_mul_in_use += 1;
      instructions.pop();
    } else {
      res_station_delays += 1;
    }
    break;
  case LW:
  case FLW:
    if (config.eff_addr_buffer > res_station_use.eff_addr_in_use) {
      if (RATs.i_rat[inst.rs1].rob_point) {
        inst.waiting_on_rs1 = true;
        inst.rs1 = RATs.i_rat[inst.rs1].rob_index;
      } else {
        inst.waiting_on_rs1 = false;
      }
      ROB_index = ROB_INDEX_STACK.top();
      ROB_INDEX_STACK.pop();
      switch (inst.op) {
      case LW:
        RATs.i_rat[inst.rd].rob_index = ROB_index;
        RATs.i_rat[inst.rd].rob_point = true;
        break;
      case FLW:
        RATs.f_rat[inst.rd].rob_index = ROB_index;
        RATs.f_rat[inst.rd].rob_point = true;
        break;
      default:
        break;
      }
      inst.issue_cycle = cycle;
      ROB.push_back(ROB_Entry(inst, inst.op, ROB_index));
      set_reqs_in_ROB(inst.op, ROB_index);
      res_station_use.rob_in_use += 1;
      res_station_use.eff_addr_in_use += 1;
      instructions.pop();
    } else {
      res_station_delays += 1;
    }
    break;
  case SW:
  case FSW:
    if (config.eff_addr_buffer > res_station_use.eff_addr_in_use) {
      inst.waiting_on_rs1 = false;
      if (RATs.i_rat[inst.rs2].rob_point) {
        inst.waiting_on_rs2 = true;
        inst.rs2 = RATs.i_rat[inst.rs2].rob_index;
      } else {
        inst.waiting_on_rs2 = false;
      }
      ROB_index = ROB_INDEX_STACK.top();
      ROB_INDEX_STACK.pop();
      inst.issue_cycle = cycle;
      ROB.push_back(ROB_Entry(inst, inst.op, ROB_index));
      set_reqs_in_ROB(inst.op, ROB_index);
      res_station_use.rob_in_use += 1;
      res_station_use.eff_addr_in_use += 1;
      instructions.pop();
    } else {
      res_station_delays += 1;
    }
    break;
  case BEQ:
  case BNE:
    if (config.ints_buffer > res_station_use.int_in_use) {

      if (RATs.i_rat[inst.rs1].rob_point) {
        inst.waiting_on_rs1 = true;
        inst.rs1 = RATs.i_rat[inst.rs1].rob_index;
      } else {
        inst.waiting_on_rs1 = false;
      }
      if (RATs.i_rat[inst.rs2].rob_point) {
        inst.waiting_on_rs2 = true;
        inst.rs2 = RATs.i_rat[inst.rs2].rob_index;
      } else {
        inst.waiting_on_rs2 = false;
      }
      ROB_index = ROB_INDEX_STACK.top();
      ROB_INDEX_STACK.pop();
      inst.issue_cycle = cycle;
      ROB.push_back(ROB_Entry(inst, inst.op, ROB_index));
      set_reqs_in_ROB(inst.op, ROB_index);
      res_station_use.rob_in_use += 1;
      res_station_use.int_in_use += 1;
      instructions.pop();
    } else {
      res_station_delays += 1;
    }
    break;
  default:
    cout << "INVALID INSTRUCTION\n";
    exit(1);
    break;
  }
}
// go through all instructions yet to execute and check if the source registers are ready
pair<vector<Ops>, bool> execute(Config config, RATs &RATs) {
  vector<Ops> ops_done_executing = {};
  bool res_station_bool = false;
  for (auto &rob_entry : ROB) {
    if (!rob_entry.inst.waiting_on_rs1 && !rob_entry.inst.waiting_on_rs2 && !rob_entry.execute_started) {
      // all source registers are ready and it hasnt started execution
      rob_entry.inst.execute_start = cycle;
      rob_entry.time_left -= 1;
      rob_entry.execute_started = true;
    } else if (!rob_entry.inst.waiting_on_rs1 && !rob_entry.inst.waiting_on_rs2) {
      rob_entry.time_left -= 1; // has started but isnt done yet
    } else {                    // one of the source registers isnt ready yet and need to check if they arer
      if (rob_entry.inst.waiting_on_rs1) {
        if (check_if_rob_entry_wrote_rs_yet(rob_entry.inst.rs1, RATs, true)) {
          rob_entry.inst.waiting_on_rs1 = false;
        }
      }
      if (rob_entry.inst.waiting_on_rs2) {
        if (check_if_rob_entry_wrote_rs_yet(rob_entry.inst.rs2, RATs, false)) {
          rob_entry.inst.waiting_on_rs2 = false;
        }
      }
      if (!rob_entry.inst.waiting_on_rs1 && !rob_entry.inst.waiting_on_rs2) {
        rob_entry.inst.execute_start = cycle;
        rob_entry.time_left -= 1;
        rob_entry.execute_started = true;
      } else {
        true_data_dependence_delays += 1;
      }
    }
    //if no time left then execution as ended
    if (rob_entry.time_left == 0) {
      rob_entry.executed = true;
      rob_entry.inst.execute_end = cycle;
      if (rob_entry.op == BEQ || rob_entry.op == BNE) {
        // return {rob_entry.op, true}; //branch is done executing free res station at end of cycle
        ops_done_executing.push_back(rob_entry.op);
        res_station_bool = true;
      } else if (rob_entry.op == SW || rob_entry.op == FSW) {
        ops_done_executing.push_back(rob_entry.op);
        res_station_bool = true;
        // res_stations.eff_addr_in_use -= 1;
        // return {rob_entry.op, true}; // store is done executing, free res station at end of cycle so issue doesnt happen same cycle
      }
    }
  }
  return {ops_done_executing, res_station_bool};
}
//find oldest ready load and send through mem read stage barring data mem hazards
bool mem(Config config, RATs &RATs) {
  int oldest_rob_id = -1;
  int oldest_invalid_load_due_to_store = INT32_MAX;
  int issued = INT32_MAX;
  data_mem_delay_counter(false);
  for (auto rob_entry : ROB) {
    if (rob_entry.need_mem && rob_entry.executed) {
      if (!check_store_addresses(rob_entry.inst.address, rob_entry.inst.issue_cycle) && rob_entry.inst.issue_cycle < issued) {
        oldest_rob_id = rob_entry.rob_id;
        issued = rob_entry.inst.issue_cycle;
      } else if (check_store_addresses(rob_entry.inst.address, rob_entry.inst.issue_cycle) && rob_entry.inst.issue_cycle < oldest_invalid_load_due_to_store) {
        // cout << "BRUH";
        // a store has the same address as a load. will check if this is older than the valid load but has to wait for a prev store to finish
        oldest_invalid_load_due_to_store = rob_entry.inst.issue_cycle;
      }
    }
  }
  // if (oldest_invalid_load_due_to_store != INT32_MAX && (oldest_invalid_load_due_to_store < issued)) {
  // cout << "cycle " << cycle << "\n";
  // }
  true_data_dependency_load_counter();
  if (oldest_rob_id != -1) {
    for (auto &rob_entry : ROB) {
      if (rob_entry.rob_id == oldest_rob_id) {
        if (is_older_store(rob_entry.inst.issue_cycle)) {
          return false;
        }
        if (!check_store_addresses(rob_entry.inst.address, rob_entry.inst.issue_cycle)) {
          rob_entry.inst.memory_read = cycle;
          rob_entry.need_mem = false;
          return true; // we did a mem read for a load, free res station at end of cycle so
        }
        // so issue doesnt happen same cycle
        break;
      }
    }
  }
  return false;
}
//write result to cdb and mark that its written 
void write_result(Config config, RATs &RATs) {
  int oldest_rob_id = -1;
  int issued = INT32_MAX;
  for (auto rob_entry : ROB) {
    if (rob_entry.executed && rob_entry.need_cdb && !rob_entry.wb_done && rob_entry.inst.issue_cycle < issued && !rob_entry.need_mem) {
      oldest_rob_id = rob_entry.rob_id;
      issued = rob_entry.inst.issue_cycle;
    }
  }
  if (oldest_rob_id != -1) {
    for (auto &rob_entry : ROB) {
      if (rob_entry.rob_id == oldest_rob_id) {
        rob_entry.inst.write_results = cycle;
        rob_entry.wb_done = true;
        rob_entry.wb_cycle = cycle;

        switch (rob_entry.inst.op) {
        case FMUL:
        case FDIV:
          res_stations.fp_mul_in_use -= 1;
          if (RATs.f_rat[rob_entry.inst.rd].rob_index == oldest_rob_id) {
            RATs.f_rat[rob_entry.inst.rd].rob_point = false;
            RATs.f_rat[rob_entry.inst.rd].rob_index = -1;
            RATs.f_rat[rob_entry.inst.rd].value_ready_cycle = cycle;
          }
          break;
        case FSUB:
        case FADD:
          res_stations.fp_add_in_use -= 1;
          if (RATs.f_rat[rob_entry.inst.rd].rob_index == oldest_rob_id) {
            RATs.f_rat[rob_entry.inst.rd].rob_point = false;
            RATs.f_rat[rob_entry.inst.rd].rob_index = -1;
            RATs.f_rat[rob_entry.inst.rd].value_ready_cycle = cycle;
          }
          break;
        case ADD:
        case SUB:
          res_stations.int_in_use -= 1;
          if (RATs.i_rat[rob_entry.inst.rd].rob_index == oldest_rob_id) {
            RATs.i_rat[rob_entry.inst.rd].rob_point = false;
            RATs.i_rat[rob_entry.inst.rd].value_ready_cycle = cycle;
            RATs.i_rat[rob_entry.inst.rd].rob_index = -1;
          }
          break;
        case FLW:
          res_stations.eff_addr_in_use -= 1;
          if (RATs.f_rat[rob_entry.inst.rd].rob_index == oldest_rob_id) {
            RATs.f_rat[rob_entry.inst.rd].rob_point = false;
            RATs.f_rat[rob_entry.inst.rd].rob_index = -1;
            RATs.f_rat[rob_entry.inst.rd].value_ready_cycle = cycle;
          }
          break;
        case LW:
          res_stations.eff_addr_in_use -= 1;
          if (RATs.i_rat[rob_entry.inst.rd].rob_index == oldest_rob_id) {
            RATs.i_rat[rob_entry.inst.rd].rob_point = false;
            RATs.i_rat[rob_entry.inst.rd].value_ready_cycle = cycle;
            RATs.i_rat[rob_entry.inst.rd].rob_index = -1;
          }
          break;
        default:
          break;
        }
      }
    }
  }
}
//commit the oldest operation and remove from rob. print out the timing cycles
pair<int, Ops> commit(Config config, RATs &RATs) {
  int oldest_rob_id = -1;
  int issued = INT32_MAX;
  int address = -1;
  Ops return_op = NULL_OP;
  for (auto rob_entry : ROB) {

    if (rob_entry.inst.issue_cycle < issued) {
      oldest_rob_id = rob_entry.rob_id;
      issued = rob_entry.inst.issue_cycle;
    }
  }
  for (auto it = ROB.begin(); it != ROB.end(); it++) {
    if (it->rob_id == oldest_rob_id) {
      if (it->executed && (it->wb_done || !it->need_cdb)) {
        print_instruction_cycles(*it);
        return_op = it->inst.op;
        address = it->inst.address;
        ROB_INDEX_STACK.push(it->rob_id);

        ROB.erase(it);
        res_stations.rob_in_use -= 1;
        return {address, return_op};
      }
      return {address, NULL_OP};
    }
  }
  return {address, NULL_OP};
}
//set params for specific instructions
void set_reqs_in_ROB(Ops op, int rob_index) {
  for (auto &rob_entry : ROB) {
    if (rob_entry.rob_id == rob_index) {
      switch (op) {
      case FLW:
      case LW:
        rob_entry.need_mem = true;
        break;
      case FSW:
      case SW:
      case BEQ:
      case BNE:
        rob_entry.need_cdb = false;
        break;
      default:
        break;
      }
      break;
    }
  }
}
//check if the instruction that another is waiting on has written yet
bool check_if_rob_entry_wrote_rs_yet(int rob_id, RATs &RATs, bool is_rs1) {
  for (const auto &rob_entry : ROB) {
    if (rob_entry.rob_id == rob_id) {
      // If the ROB entry has finished its write-back then clear RAT mapping
      if (rob_entry.wb_done && rob_entry.wb_cycle != cycle) {
        int rd = rob_entry.inst.rd;
        // Only clear RAT entries for instructions that actually write a destination
        if (rd >= 0) {
          switch (rob_entry.op) {
          case FADD:
          case FSUB:
          case FMUL:
          case FDIV:
          case FLW:
            if (RATs.f_rat[rd].rob_index == rob_id) {
              RATs.f_rat[rd].rob_point = false;
            }
            break;
          case ADD:
          case SUB:
          case LW:
            if (RATs.i_rat[rd].rob_index == rob_id) {
              RATs.i_rat[rd].rob_point = false;
            }
            break;
          default:
            break;
          }
        }
        return true;
      } else {
        return false;
      }
      return false;
    }
  }
  // If we didn't find a matching ROB entry then treat as ready
  return true;
}
//print the instruction cycles and info
void print_instruction_cycles(ROB_Entry rob_entry) {
  cout << setfill(' ') << setw(21) << left << rob_entry.inst.original_instruction << " ";
  cout << setfill(' ') << setw(6) << right << rob_entry.inst.issue_cycle << " ";
  cout << setfill(' ') << setw(3) << rob_entry.inst.execute_start << " -";
  cout << setfill(' ') << setw(3) << rob_entry.inst.execute_end << " ";
  if (rob_entry.inst.memory_read != -1) {
    // data_mem_conflict_delays  += ((rob_entry.inst.memory_read - rob_entry.inst.execute_end) - 1);
    cout << setfill(' ') << setw(6) << rob_entry.inst.memory_read << " ";
  } else {
    cout << "       ";
  }
  if (rob_entry.inst.write_results != -1) {
    cout << setfill(' ') << setw(6) << rob_entry.inst.write_results << " ";
  } else {
    cout << "       ";
  }
  cout << setfill(' ') << setw(7) << cycle << "\n";
  if (rob_entry.inst.memory_read != -1) {
    // true_data_dependence_delays += (rob_entry.inst.memory_read - rob_entry.inst.execute_end) - 1; //something here
    // need to distinguish between the 2. data dependence was 83, should be 86. now its 93. the mem dependence is 7.
   
  }

 
}

bool check_store_addresses(int address, int issue_cycle) { // true if store is using address, false otherwise
  for (auto rob_entry : ROB) {
    if (rob_entry.inst.address == address && (rob_entry.op == FSW || rob_entry.op == SW) && issue_cycle > rob_entry.inst.issue_cycle) {
      // addresses match, is a store instruction and the load comes after the store
      return true;
    }
  }
  return false;
}
//loads cant go to mem stage if an older store hasnt executed yet ¯\_(ツ)_/¯ (for some reason)
bool is_older_store(int issue_cycle) {
  for (auto rob_e : ROB) {
    if ((rob_e.op == SW || rob_e.op == FSW) && rob_e.inst.issue_cycle < issue_cycle && !rob_e.executed) {
      true_data_dependence_delays += 1;
      return true;
    }
  }
  return false;
}

//counts the true dependencies 
void true_data_dependency_load_counter() {
  map<int, int> load_addresses;  // cycle -> address
  map<int, int> store_addresses; // cycle -> address
  for (auto &rob_entry : ROB) {
    if ((rob_entry.op == FLW || rob_entry.op == LW) && rob_entry.need_mem && rob_entry.executed) {
      load_addresses[rob_entry.inst.issue_cycle] = rob_entry.inst.address;
    }
    if (rob_entry.op == SW || rob_entry.op == FSW) {
      store_addresses[rob_entry.inst.issue_cycle] = rob_entry.inst.address;
    }
  }
  if (load_addresses.empty() || store_addresses.empty()) {
    return;
  }
  for (auto &[load_cycle, load_addr] : load_addresses) {
    auto it = store_addresses.lower_bound(load_cycle);
    bool dependency_found = false;
    for (auto sit = store_addresses.begin(); sit != it; ++sit) {
      if (sit->second == load_addr) {
        dependency_found = true;
        break;
      }
    }
    if (dependency_found)
      true_data_dependence_delays++;
  }
}
// counts data memory delays
void data_mem_delay_counter(bool was_store_committed) {
  int loads_ready_for_mem = 0;
  for (auto rob_entry : ROB) {
    if ((rob_entry.op == FLW || rob_entry.op == LW) && rob_entry.need_mem && rob_entry.executed) {
      if (was_store_committed) {
        loads_ready_for_mem += 1;
      } else if (!check_if_load_is_waiting_on_store(rob_entry.inst.issue_cycle, rob_entry.inst.address)) {
        loads_ready_for_mem += 1;
      }
    }
  }
  if (loads_ready_for_mem >= 1 && was_store_committed) {
    data_mem_conflict_delays += loads_ready_for_mem;
    // cout << "adding " << loads_ready_for_mem << " conflict delays on cycle " << cycle << "\n";
  } else if (loads_ready_for_mem > 1) {
    data_mem_conflict_delays += loads_ready_for_mem - 1;
    // cout << "adding " << loads_ready_for_mem-1 << " conflict delays on cycle " << cycle << "\n";
  }
}

bool check_if_load_is_waiting_on_store(int issue_cycle, int address) {
  int earliest_matching_address_issue = INT32_MAX;
  for (auto rob_entry : ROB) {
    if ((rob_entry.op == SW || rob_entry.op == FSW) && rob_entry.inst.issue_cycle < issue_cycle && rob_entry.inst.address == address && rob_entry.inst.issue_cycle < earliest_matching_address_issue) {
      earliest_matching_address_issue = rob_entry.inst.issue_cycle;
      return true; // an earlier store exists for a load meaning it is not having a data mem conflict since it is waiting on store to finish
    }
  }

  return false;
}

/*
Really early psuedocode that i write on how i thought i should complete this. im leaving it since its pretty accurate
issue:
check for space in rob and res station, if either full stall
set RATs.x_rat[rd].rob_id = first_open_rob_index (maybe want to have an instruction counter that can be used to find first instruction)
check if RATs.x_rat[ins.rs1 or ins.rs2] is pointing to rob
if so set waiting_on_rsx_to_true and set rs1 = rob_index
execute:
start at beginning of ROB
check if instruction is ALL_DONE to skip
check if RATs.x_rat[ROB.ins.waiting_on_rs1] if so check if that rob wb has been done
if still waiting for either rs then continue else time_left -= 1
check if ROB.time_left <= 0
deal with counters here and freeing reservation stations of stores that execution completed
write back:
loop through rob
if done && !wb_done && !need_mem && need_cdb
  set wb = true
  set value in rat (maybe not needed)
  give back reservation stations of arithmetic instructions
mem read:
if mem_need
  check if write port is being used by a sw commit, free load reservation stations once done here
commit:
if all_done and wb is done
  evict from rob

loads leave res station after the mem stage (same stage as writeback)
arithmetic leave res station after write to cdb stage (same cycle as write)
stores leave res station after execution (1 cycle after execution finishes (execution cycle starts/ends 3-3
then it will still be at cycle 3 but gone by cycle 4 (something can replace it on cycle 4) ))

stores committing use the same write port as other instructions cdb write and
take priority over them

load store address dependencies:
if a load happens after a store, that store must be waiting to commit (ie done with executing)
load will stall between execute and memory stage

add the stage cycles to the instruction and update them through the stages, once committed then print

maybe make a function that will just find the oldest load and then see if a store has the same address and is older for data mem conflicts - actually true dependence???????
data mem seems to be when
*/
