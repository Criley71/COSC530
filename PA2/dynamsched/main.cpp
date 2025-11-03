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
public:
  Ops op;
  int rd;
  int rs1;
  int rs2;
  int address;
  bool is_branch = false;
  int latency;

  Instruction(Ops op_, int rd_, int rs1_, int rs2_, int addr, bool is_branch_, int lat) {
    op = op_;
    rd = rd_;
    rs1 = rs1_;
    rs2 = rs2_;
    address = addr;
    latency = lat;
    is_branch = is_branch_;
  }
};


class RegisterAliasTable{
  public:
  int rat_id = -1;
  int val = -1;
  
};

class RATs{
  RegisterAliasTable int_rats[32];
  RegisterAliasTable fp_rats[32];
};

class ReservationStations {
  int eff_addr_size;
  int fp_add_size;
  int fp_mul_size;
  int int_size;
  int reorder_size;

  int eff_addr_in_use;
  int fp_add_in_use;
  int fp_mul_in_use;
  int int_in_use;
  int reorder_in_use;
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
  while (!instructions.empty()) {

    cout << "OP:    " << instructions.front().op << "\n";
    cout << "rd:    " << instructions.front().rd << "\n";
    cout << "rs1:   " << instructions.front().rs1 << "\n";
    cout << "rs2:   " << instructions.front().rs2 << "\n";
    cout << "addr:  " << instructions.front().address << "\n";
    cout << "btar:  " << instructions.front().is_branch << "\n\n";
    instructions.pop();
  }

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
    instruction_list.push(Instruction(op, rd, rs1, rs2, address, is_branch, latency));
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
*/