#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifndef DC_H
#define DC_H
using namespace std;
class Cache_Block {
public:
  int tag = -1;
  int time_last_used = -1;
  bool dirty = false;
  int pfn = -1;
  string physical_address = "";
  Cache_Block(int t, int u, bool d, int pfn, string pa);
};

class DC {
public:
  int set_count;
  int set_size;
  int line_size;
  bool no_write_allo;
  int index_bit_size;
  int offset_bit_size;
  vector<vector<Cache_Block>> data_cache;
  DC(int sc, int ss, int ls, bool wa, int ibs, int obs);
  pair<bool, string> insert_to_cache(int dc_index, int dc_tag, int time, bool dirty, int pfn, string phys_addr);
  bool check_cache(int dc_index, int dc_tag, int time, bool is_write, int pfn , bool page_fault);
  void invalidate_bc_l2_eviction(int dc_index, int dc_tag, double& l2_refs, int& memory_refs);
  bool evict_given_pfn(int pfn, int &disk_ref, int &mem_refs, int &page_refs, bool l2_enabled, double &l2_hits);
  bool is_dirty(int index, int tag, int pfn);
};

#endif