#include "config.h"
#include <iostream>
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
  FDIV  // 11
};

class Instruction {
public:
  Ops op;
  string rd;
  string rs1;
  string rs2;
  int address;
  bool is_branch = false;
  string branch_target;
  Instruction(Ops op, string rd, string rs1, string rs2, int address, string branch_target) {
    this->op = op;
    this->rd = rd;
    this->rs1 = rs1;
    this->rs2 = rs2;
    this->address = address;
    this->branch_target = branch_target;
    if (branch_target != "") {
      this->is_branch = true;
    }
  }
};

void dynam_schedule(Config config);
vector<Instruction> get_instructions(Config config); //DONT NEED TO PASS CONFIG
void issue(Config config);
void execute(Config config);
void write_result(Config config);

int main() {
  Config config = Config();
  config.print_config();
  // vector<Instruction> instructions = get_instructions(config);
  // cout << "\n\n";
  // for (int i = 0; i < instructions.size(); i++) {
  //   cout << "OP:    " << instructions[i].op << "\n";
  //   cout << "rd:    " << instructions[i].rd << "\n";
  //   cout << "rs1:   " << instructions[i].rs1 << "\n";
  //   cout << "rs2:   " << instructions[i].rs2 << "\n";
  //   cout << "addr:  " << instructions[i].address << "\n";
  //   cout << "btar:  " << instructions[i].branch_target << "\n\n";
  // }
  dynam_schedule(config);
}

vector<Instruction> get_instructions(Config config) {
  string instruction;
  vector<Instruction> instruction_list;
  string rd;
  string rs1;
  string rs2;
  while (getline(cin, instruction)) {
    cout << instruction << "\n";

    regex delimiters("[ ,():\\t]+");
    vector<string> pi = {}; // pi = parsed instructions
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
    if (pi[0] == "lw") {
      instruction_list.push_back(Instruction(LW, pi[1], "", "", stoi(pi[4], 0, 10), ""));
    } else if (pi[0] == "flw") {
      instruction_list.push_back(Instruction(FLW, pi[1], "", "", stoi(pi[4], 0, 10), ""));
    } else if (pi[0] == "sw") {
      instruction_list.push_back(Instruction(SW, "", "", pi[1], stoi(pi[4], 0, 10), ""));
    } else if (pi[0] == "fsw") {
      instruction_list.push_back(Instruction(FSW, "", "", pi[1], stoi(pi[4], 0, 10), ""));
    } else if (pi[0] == "add") {
      instruction_list.push_back(Instruction(ADD, pi[1], pi[2], pi[3], -1, ""));
    } else if (pi[0] == "sub") {
      instruction_list.push_back(Instruction(SUB, pi[1], pi[2], pi[3], -1, ""));
    } else if (pi[0] == "beq") {
      instruction_list.push_back(Instruction(BEQ, "", pi[1], pi[2], -1, pi[3]));
    } else if (pi[0] == "bne") {
      instruction_list.push_back(Instruction(BNE, "", pi[1], pi[2], -1, pi[3]));
    } else if (pi[0] == "fadd.s") {
      instruction_list.push_back(Instruction(FADD, pi[1], pi[2], pi[3], -1, ""));
    } else if (pi[0] == "fsub.s") {
      instruction_list.push_back(Instruction(FSUB, pi[1], pi[2], pi[3], -1, ""));
    } else if (pi[0] == "fmul.s") {
      instruction_list.push_back(Instruction(FMUL, pi[1], pi[2], pi[3], -1, ""));
    } else if (pi[0] == "fdiv.s") {
      instruction_list.push_back(Instruction(FDIV, pi[1], pi[2], pi[3], -1, ""));
    }
  }
  return instruction_list;
}

void dynam_schedule(Config config) {
  cout << "                    Pipeline Simulation\n";
  cout << "-----------------------------------------------------------\n";
  cout << "                                      Memory Writes\n";
  cout << "     Instruction      Issues Executes  Read  Result Commits\n";
  cout << "--------------------- ------ -------- ------ ------ -------\n";
  get_instructions(config);
}