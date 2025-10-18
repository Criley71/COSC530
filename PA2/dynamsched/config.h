#ifndef CONFIG_H
#define CONFIG_H
#include <iostream>
#include <vector>
#include <fstream>
using namespace std;
class Config{
  public:
  int eff_addr_buffer;
  int fp_adds_buffer;
  int fp_muls_buffer;
  int ints_buffer;
  int reorder_buffer;

  int fp_add_latency;
  int fp_sub_latency;
  int fp_mul_latency;
  int fp_div_latency;

  Config();
  void print_config();
};

#endif