#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "dc.h"
#ifndef L2_H
#define L2_H
using namespace std;

class L2_block {
public:
  uint index = -1; // placeholder values
  uint tag = -1;
  int time_last_used = -1;
  bool dirty = false;
  int pfn;
  int dc_ratio;
  bool valid = false;
  vector<pair<uint64_t, uint64_t>> dc_index_and_tags;
  L2_block(uint64_t i, uint64_t t, int a, bool d, int pf, int dcr, bool v);
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
  pair<bool, vector<pair<uint64_t,uint64_t>>> insert_to_l2(uint64_t l2_index, uint64_t l2_tag, int time, bool dirty , int pfn, int &memory_refs, uint64_t dc_i, uint64_t dc_t);
  void insert_address_to_block(uint64_t index, uint64_t tag, uint64_t dc_i, uint64_t dc_t, DC &cache, int timer);
  bool check_l2(uint64_t l2_index, uint64_t l2_tag, int time, bool dirty, int pfn, bool page_fault);
  bool check_if_index_is_full(int index);
  void update_used_time(uint64_t l2_index, uint64_t l2_tag, int timer);
  void update_dirty_bit(uint64_t l2_index, uint64_t l2_tag, int timer);
  pair<bool, vector<pair<int,int>>> evict_given_pfn(int pfn, int &memory_refs, double &l2_hits, DC &dc);
  void update_the_dc_ind_tag(uint64_t l2_index, uint64_t l2_tag, uint64_t dc_index, uint64_t dc_tag, DC &cache, int pfn, int &timer);
};

#endif