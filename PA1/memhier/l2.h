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
  int dc_tag = -1;
  int dc_index = -1;
  string dc_address = "";
  L2_block(int i, int t, int a, int db, int pf, int dct, int dci, string dca);
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
  pair<bool, string> insert_to_l2(int l2_index, int l2_tag, int time, int dirty_bit, int pfn, int dc_index, int dc_tag, string dc_address, int &counter);
  bool check_l2(int l2_index, int l2_tag, int time, int dirty_bit, int pfn, bool page_fault, int dc_index, int dc_tag);
  bool check_if_index_is_full(int l2_index);
  uint64_t l2_index_and_tag_evicted_phys_address(int l2_index);
  pair<int, int> get_dc_index_tag(int l2_index, int l2_tag);
  vector<pair<int,int>> evict_entries_by_pfn(int pfn, int&mem_refs);
  void update_access_time(int l2_index, int l2_tag, int time);
  void update_dirty_bit(int l2_index, int l2_tag, int time); //on dc eviction, update dirty bit if was dirty does that mean l2 time needs to be updated?
};

#endif