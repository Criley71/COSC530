#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#ifndef DC_H
#define DC_H
using namespace std;
class Cache_block {
public:
  int tag = -1;
  int index = -1;
  Cache_block(int i, int t);
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
  void insert_to_cache(int dec_dc_index, int dec_dc_tag);
  bool check_cache(int dec_dc_index, int dec_dc_tag);
};

#endif