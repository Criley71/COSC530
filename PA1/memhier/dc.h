#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifndef DC_H
#define DC_H
using namespace std;
class Cache_block {
public:
  int index = -1; // all place holder values for initialization
  int tag = -1;
  int time_last_accessed = -1;
  int dirty_bit = -1;
  Cache_block(int i, int t, int a, int db);
};

class DC {
public:
  int set_count;
  int set_size;
  int line_size;
  bool write_allo_write_back;
  int index_bit_size;
  int offset_bit_size;
  bool associative;
  vector<vector<Cache_block>> data_cache;
  DC(int sc, int ss, int ls, bool wawb, int ibs, int obs);
  void insert_to_cache(int dec_dc_index, int dec_dc_tag, int time, int dirty_bit);
  bool check_cache(int dec_dc_index, int dec_dc_tag, int time, bool is_write);
};

#endif