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
  int time_last_used = -1;
  bool dirty = false;
  int pfn;
  int dc_ratio;
  vector<pair<int, int>> dc_index_and_tags;
  L2_block(int i, int t, int a, bool d, int pf, int dcr);
};

class L2 {
  public:
  int set_count;
  int set_size;
  int line_size;
  bool write_allo_write_back;
  int index_bit_size;
  int offset_bit_size;
  int dc_ratio;
  vector<vector<L2_block>> l2_cache;
  L2(int sc, int ss, int ls, bool wa, int ibs, int obs, int dcr);
  pair<bool, vector<pair<int,int>>> insert_to_l2(int l2_index, int l2_tag, int time, bool dirty , int pfn, int &memory_refs, int dc_i, int dc_t);
  void insert_address_to_block(int index, int tag, int dc_i, int dc_t);
  bool check_l2(int l2_index, int l2_tag, int time, bool dirty, int pfn, bool page_fault);
  bool check_if_index_is_full(int index);
  void update_used_time(int l2_index, int l2_tag, int timer);
  void update_dirty_bit(int l2_index, int l2_tag, int timer);

};

#endif