#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifndef L2_H
#define L2_H
using namespace std;

class L2_block {
public:
  int index = -1; // placeholder values
  int tag = -1;
  int time_last_accessed = -1;
  int dirty_bit = -1;
  int pfn;
  L2_block(int i, int t, int a, int db, int pf);
};

class L2 {
public:
  int set_count;
  int set_size;
  int line_size;
  bool write_allo_write_back;
  int index_bit_size;
  int offset_bit_size;
  bool associative;

  vector<vector<L2_block>> l2_cache;
  L2(int sc, int ss, int ls, bool wawb, int ibs, int obs);
  void insert_to_l2(int dec_l2_index, int dec_l2_tag, int time, int dirty_bit, int pfn);
  bool check_l2(int dec_l2_index, int dec_l2_tag, int time, int dirty_bit, int pfn, bool page_fault);
};

#endif