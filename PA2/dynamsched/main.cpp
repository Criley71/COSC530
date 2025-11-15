#include "config.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <queue>
#include <regex>
#include <string>
#include <vector>
using namespace std;

enum Ops {
  LW,   // 0
  FLW,  // 1
  SW,   // 2
  FSW,  // 3
  ADD,  // 4
  SUB,  // 5
  BEQ,  // 6
  BNE,  // 7
  FADD, // 8
  FSUB, // 9
  FMUL, // 10
  FDIV, // 11
  NULL_OP
};

class Instruction {
public:        // the registers will just be ints of the register number (rd=6 means rd=f6 or rd=x6 (depends on op))
  Ops op;      // op type
  int rd;      // destination register
  int rs1;     // source register 1
  int rs2;     // source register 2
  int address; //
  bool is_branch = false;
  int latency;
  bool waiting_on_rs1;
  bool waiting_on_rs2;
  int issue_cycle = -1;
  int execute_start = -1;
  int execute_end = -1;
  int memory_read = -1;
  int write_results = -1;
  int commits = -1;
  string original_instruction;
  Instruction() {
  }
  Instruction(Ops op_, int rd_, int rs1_, int rs2_, int addr, bool is_branch_, int lat, bool wrs1, bool wrs2, string oi) {
    op = op_;
    rd = rd_;
    rs1 = rs1_;
    rs2 = rs2_;
    address = addr;
    latency = lat;
    is_branch = is_branch_;
    waiting_on_rs1 = wrs1;
    waiting_on_rs2 = wrs2;
    original_instruction = oi;
  }
};

class RegisterAliasTableEntry {
public:
  bool rob_point; // is it currently point to a rob
  int rob_index;  // if so where (-1 means no)

  RegisterAliasTableEntry(bool rp, int ri) {
    rob_point = rp;
    rob_index = ri;
  }
};

/*
What the RAT does:
when an instruction is getting source operands it goes to the rat
if the value is in the rat that means its being calculated and it
points to what rob will
*/

class RATs {
public:
  vector<RegisterAliasTableEntry> f_rat;
  vector<RegisterAliasTableEntry> i_rat;
  RATs() {
    for (int i = 0; i < 32; i++) {
      f_rat.push_back(RegisterAliasTableEntry(false, -1));
      i_rat.push_back(RegisterAliasTableEntry(false, -1));
    }
  }
};

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
  }
};

// FUNCTIONS
void dynam_schedule(Config config);
queue<Instruction> get_instructions(Config config);
int register_parser(string reg);
void set_reqs_in_ROB(Ops op, int rob_index);
void issue(Config config, queue<Instruction> &instructions, RATs &RATs, ReservationStationUsage &res_station_use);
void execute(Config config, RATs &RATs);
void mem(Config config, RATs &RATs);
void write_result(Config config, RATs &RATs);
Ops commit(Config config, RATs &RATs);
bool check_if_rob_entry_wrote_rs_yet(int rob_id, RATs &RATs, bool is_rs1);
void print_instruction_cycles(ROB_Entry rob_entry);
// GLOBAL VARIABLES
int cycle = 1;
stack<int> ROB_INDEX_STACK;
vector<ROB_Entry> ROB;
vector<int> load_addresses;
vector<int> store_addresses;
RATs rats = RATs();
ReservationStationUsage res_stations = ReservationStationUsage();

int main() {
  Config config = Config();
  config.print_config();
  for (int i = config.reorder_buffer; i > 0; i--) {
    ROB_INDEX_STACK.push(i);
  }
  // queue<Instruction> instructions = get_instructions(config);
  //cout << "\n\n";
  // while (!instructions.empty()) {

  // cout << "OP:    " << instructions.front().op << "\n";
  //    cout << "rd:    " << instructions.front().rd << "\n";
  //    cout << "rs1:   " << instructions.front().rs1 << "\n";
  //    cout << "rs2:   " << instructions.front().rs2 << "\n";
  // cout << "addr:  " << instructions.front().address << "\n";
  //  cout << "btar:  " << instructions.front().is_branch << "\n\n";
  // instructions.pop();
  // }

  dynam_schedule(config);
}

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
    // cout << instruction << "\n";

    regex delimiters("[ ,():\\t]+");
    vector<string> pi = {}; // pi = parsed instructions, but also not all the way parsed
    sregex_token_iterator it(instruction.begin(), instruction.end(), delimiters, -1);
    sregex_token_iterator end;
    while (it != end) {
      if (!it->str().empty()) {
        pi.push_back(it->str());
      }
      ++it;
    }
    // ARITHMETIC INSTRUCTION PARSED VECTOR FORMAT: [op, rd, rs1, rs2]
    // LOAD INSTRUCTION PARSED VECTOR FORMAT:       [op, rd, 34, x1, address] (34 and x1 dont matter, they are just memory addresses so no hazards)
    // STORE INSTRUCTION PARSED VECTOR FORMAT:      [op, rs2, 37, x1, address] (37 and x1 dont matter, they are just memory addresses so no hazards)
    // BRANCH INSTRUCTION PARSED VECTOR FORMAT:     [op, rs1, rs2, address]
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
    bool is_branch = false;
    int latency = 1;
    cout << endl;
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
      is_branch = true;
      rs1 = register_parser(pi[1]);
      rs2 = register_parser(pi[2]);
      // rd = register_parser(pi[3]);
      break;
    default:
      break;
    }

    instruction_list.push(Instruction(op, rd, rs1, rs2, address, is_branch, latency, false, false, instruction));
  }
  if (instruction_list.empty()) {
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
  Ops commit_op = NULL_OP;
  while (!instructions.empty() || !ROB.empty()) {
    commit_op = NULL_OP;
   // cout << "cycle " << cycle << "\n";
    //cout << "test 0\n";
    commit_op = commit(config, rats);
    //cout << "test 1\n";
    write_result(config, rats);
    //cout << "test 2\n";
    if (commit_op != FSW && commit_op != SW) {
     //cout << "test 3\n";
      mem(config, rats);
    }
    //cout << "test 4\n";
    execute(config, rats);
   // cout << "test 5\n";
    issue(config, instructions, rats, res_stations);
   // cout << "test 6\n";
    cycle += 1;
    if(cycle == 19){
      exit(1);
    }
  }
}

int register_parser(string reg) {
  if (reg.empty()) return -1;
  return stoi(reg.substr(1));
}

void issue(Config config, queue<Instruction> &instructions, RATs &RATs, ReservationStationUsage &res_station_use) {
  if (res_station_use.rob_in_use == config.reorder_buffer || instructions.empty()) {
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
    }
    break;
  case SW:
  case FSW:
    if (config.eff_addr_buffer > res_station_use.eff_addr_in_use) {
      switch (inst.op) {
      case SW:
        if (RATs.i_rat[inst.rs1].rob_point) {
          inst.waiting_on_rs1 = true;
          inst.rs1 = RATs.i_rat[inst.rs1].rob_index;
        } else {
          inst.waiting_on_rs1 = false;
        }
        break;
      case FSW:
        if (RATs.f_rat[inst.rs1].rob_point) {
          inst.waiting_on_rs1 = true;
          inst.rs1 = RATs.f_rat[inst.rs1].rob_index;
        } else {
          inst.waiting_on_rs1 = false;
        }
        break;
      default:
        break;
      }
      if (RATs.i_rat[inst.rs2].rob_point) {
        inst.waiting_on_rs2 = true;
        inst.rs2 = RATs.i_rat[inst.rs1].rob_index;
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
    }
    break;
  default:
    cout << "INVALID INSTRUCTION\n";
    exit(1);
    break;
  }
}

void execute(Config config, RATs &RATs) {
  for (auto &rob_entry : ROB) {
    if (!rob_entry.inst.waiting_on_rs1 && !rob_entry.inst.waiting_on_rs2 && !rob_entry.execute_started) {
      // all source registers are ready and it hasnt been

      rob_entry.inst.execute_start = cycle;
     // cout << rob_entry.inst.execute_start << "\n";
      rob_entry.time_left -= 1;
      rob_entry.execute_started = true;
    } else if (!rob_entry.inst.waiting_on_rs1 && !rob_entry.inst.waiting_on_rs2) {
      rob_entry.time_left -= 1;
    } else {
      if (rob_entry.inst.waiting_on_rs1) {
        //cout << rob_entry.inst.rs1 << "<-\n";
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
      }
    }

    if (rob_entry.time_left == 0) {
      rob_entry.executed = true;
      rob_entry.inst.execute_end = cycle;
      if (rob_entry.op == BEQ || rob_entry.op == BNE) {
        res_stations.int_in_use -= 1;
      } else if (rob_entry.op == SW || rob_entry.op == FSW) {
        res_stations.eff_addr_in_use -= 1;
      }
    }
  }
}

void mem(Config config, RATs &RATs) {
  int oldest_rob_id = -1;
  int issued = INT32_MAX;
  for (auto rob_entry : ROB) {
    if (rob_entry.need_mem && rob_entry.executed && rob_entry.inst.issue_cycle < issued) {
      oldest_rob_id = rob_entry.rob_id;
    }
  }
  if (oldest_rob_id != -1) {
    for (auto &rob_entry : ROB) {
      if (rob_entry.rob_id == oldest_rob_id) {
        rob_entry.inst.memory_read = cycle;
        rob_entry.need_mem = false;
        res_stations.eff_addr_in_use -= 1;
      }
    }
  }
}

void write_result(Config config, RATs &RATs) {
  int oldest_rob_id = -1;
  int issued = INT32_MAX;
  for (auto rob_entry : ROB) {
    if (rob_entry.executed && rob_entry.need_cdb && !rob_entry.wb_done && rob_entry.inst.issue_cycle < issued && !rob_entry.need_mem) {
      oldest_rob_id = rob_entry.rob_id;
    }
  }
  if (oldest_rob_id != -1) {
    for (auto &rob_entry : ROB) {
      if (rob_entry.rob_id == oldest_rob_id) {
        rob_entry.inst.write_results = cycle;
       // cout << "rob id wb set true here " << rob_entry.rob_id << "\n";
        rob_entry.wb_done = true;

        switch (rob_entry.inst.op) {
        case FMUL:
        case FDIV:
          res_stations.fp_mul_in_use -= 1;
          break;
        case FSUB:
        case FADD:
          res_stations.fp_add_in_use -= 1;
          break;
        case ADD:
        case SUB:
          res_stations.int_in_use -= 1;
          break;
        default:
          break;
        }
      }
    }
  }
}

Ops commit(Config config, RATs &RATs) {
  int oldest_rob_id = -1;
  int issued = INT32_MAX;
  Ops return_op = NULL_OP;
  for (auto rob_entry : ROB) {
    
    if (rob_entry.inst.issue_cycle < issued) {
      oldest_rob_id = rob_entry.rob_id;
      issued = rob_entry.inst.issue_cycle;
    }
  }
  for (auto it = ROB.begin(); it != ROB.end();it++) {
    if (it->rob_id == oldest_rob_id) {
      if (it->executed && (it->wb_done || !it->need_cdb)) {
        cout << it->inst.original_instruction << " " << it->inst.issue_cycle << " " << it->inst.execute_start << "-" << it->inst.execute_end << " "  << it->inst.memory_read << " " << it->inst.write_results << " " << cycle << "\n";
        return_op = it->inst.op;
        ROB.erase(it);
        res_stations.rob_in_use -= 1;
        return return_op;
      }
      return NULL_OP;
    }
  }
  return NULL_OP;
}

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

bool check_if_rob_entry_wrote_rs_yet(int rob_id, RATs &RATs, bool is_rs1) {
  for (const auto &rob_entry : ROB) {
    if (rob_entry.rob_id == rob_id) {
      // If the ROB entry has finished its write-back then clear RAT mapping
     // cout <<rob_id <<"test2\n";
      if (rob_entry.wb_done) {
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
      }
      return false;
    }else{
      return true;
    }
  }
  // If we didn't find a matching ROB entry, treat as not-yet-ready
  return false;
}

void print_instruction_cycles(ROB_Entry rob_entry){

}

/*
TODO:
figure out speculative differences from non-speculative
issue will check if a reservation station is available, if so, it will allocate it and update the RAT
read jantz notes on speculation

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
airthmetic leave res station after write to cdb stage (same cycle as write)
stores leave res station after execution (1 cycle after execution finishes (execution cycle starts/ends 3-3
then it will still be at cycle 3 but gone by cycle 4 (something can replace it on cycle 4) ))

stores committing use the same write port as other instructions cdb write and
take priority over them

load store address dependencies:
if a load happens after a store, that store must be waiting to commit (ie done with executing)
load will stall between execute and memory stage


//add the stage cycles to the instruction and update them through the stages, once committed then print
*/
