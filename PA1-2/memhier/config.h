#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <math.h>
#ifndef CONFIG_H
#define CONFIG_H
using namespace std;

class Config {
public:
  // data translation lookaside buffer
  int dtlb_set_count; // max 256
  int dtlb_set_size;
  int dltb_index_bits;
  bool dtlb_enabled;
  int dtlb_associativity;

  int page_size;
  int pt_offset_bit;
  int pt_index_bits;

  int virtual_page_count; // max 8192, must be power of 2
  int virtual_page_size;  // (bytes)
  int virtual_page_num;

  int physical_page_count; // max 1024, must be power of 2
  int physical_page_size;
  // int physical_page_bit_count;
  int physical_page_num;

  int dc_set_count; // max 8192
  int dc_set_size;
  int dc_associativity;
  int dc_line_size; // min 8 (bytes)
  bool dc_write_thru_no_allo;
  int dc_index_bits;
  int dc_offset_bits;

  int l2_set_count;
  int l2_set_size;
  int l2_associativity;
  int l2_line_size; // >= dc_line_size (bytes)
  bool l2_write_thru_no_allo;
  bool l2_enabled;
  int l2_index_bits;
  int l2_offset_bits;
  int counter = 0;
  bool virt_addr;
  
  int l2_to_dc_ratio;
  int pt_to_l2_ratio;
  int pt_to_dc_ratio;
  Config();
  void print_config_after_read();
  bool check_pwr_2(int val);
  
};


#endif