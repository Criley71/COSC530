#include "config.h"
#include <iostream>
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
public:    // the registers will just be ints of the register number (rd=6 means rd=f6 or rd=x6 (depends on op))
  Ops op;  // op type
  int rd;  // destination register
  int rs1; // source register 1
  int rs2; // source register 2
  int address; //
  bool is_branch = false;
  int latency;
  bool waiting_on_rs1;
  bool waiting_on_rs2;
  Instruction() {
  }
  Instruction(Ops op_, int rd_, int rs1_, int rs2_, int addr, bool is_branch_, int lat, bool wrs1, bool wrs2) {
    op = op_;
    rd = rd_;
    rs1 = rs1_;
    rs2 = rs2_;
    address = addr;
    latency = lat;
    is_branch = is_branch_;
    waiting_on_rs1 = wrs1;
    waiting_on_rs2 = wrs2;
  }
};

class RegisterAliasTableEntry {
public:
  bool rob_point; // is it currently point to a rob
  int rob_index;  // if so where (-1 means no)
  int value;      // other wise what is the value (i dont this actually matters for this sim but whatever)
  RegisterAliasTableEntry(bool rp, int ri, int v) {
    rob_point = rp;
    rob_index = ri;
    value = v;
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
      f_rat.push_back(RegisterAliasTableEntry(false, -1, -1));
      i_rat.push_back(RegisterAliasTableEntry(false, -1, -1));
    }
  }
};

class ReservationStationUsage {
  int eff_addr_in_use = 0;
  int fp_add_in_use = 0;
  int fp_mul_in_use = 0;
  int int_in_use = 0;
  int rob_in_use = 0;
};

class ROB_Entry {
  Instruction inst; // instruction
  Ops op;           // the enum op type
  int rob_id;       // rob id for tracking where the rat is pointing
  int time_left;    // latency tracker
  bool wb_done;     // write back stage done
  bool committed;   // committed stage done
  bool need_mem;    // if the op needs the memory read stage
  bool need_cdb;    // if it needs to write to the common data bus
  bool all_done;    // this rob entry is all done
  ROB_Entry(Instruction i, Ops o, int tl) {
    inst = i;
    op = o;
    time_left = tl;
    wb_done = false;
    committed = false;
    need_mem = false;
    need_cdb = true; // more need this than not so default to true
    all_done = false;
  }
};

void dynam_schedule(Config config);
queue<Instruction> get_instructions(Config config);
int register_parser(string reg);
void issue(Config config);
void execute(Config config);
void write_result(Config config);

int main() {
  Config config = Config();
  config.print_config();
  queue<Instruction> instructions = get_instructions(config);
  cout << "\n\n";
  // while (!instructions.empty()) {

  //   cout << "OP:    " << instructions.front().op << "\n";
  //   cout << "rd:    " << instructions.front().rd << "\n";
  //   cout << "rs1:   " << instructions.front().rs1 << "\n";
  //   cout << "rs2:   " << instructions.front().rs2 << "\n";
  //   cout << "addr:  " << instructions.front().address << "\n";
  //   cout << "btar:  " << instructions.front().is_branch << "\n\n";
  //   instructions.pop();
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
    cout << instruction << "\n";

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
    // BRANCH INSTRUCTION PARSED VECTOR FORMAT:     [op, rs1, rs2, branch_target]
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

    switch (op) {
    case LW:
    case FLW:
      rd = register_parser(pi[1]);
      address = stoi(pi[4]);
      break;
    case SW:
    case FSW:
      rs2 = register_parser(pi[1]);
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
      rd = register_parser(pi[3]);
      break;
    default:
      break;
    }
    instruction_list.push(Instruction(op, rd, rs1, rs2, address, is_branch, latency, false, false));
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
}

int register_parser(string reg) {
  if (reg.empty()) return -1;
  return stoi(reg.substr(1));
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
*/
