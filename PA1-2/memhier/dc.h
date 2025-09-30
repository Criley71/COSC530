#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifndef DC_H
#define DC_H
using namespace std;
class Cache_Block {
public:
  uint64_t tag = -1;
  int time_last_used = -1;
  bool dirty = false;
  int pfn = -1;
  bool valid = false;
  uint64_t physical_address;
  Cache_Block(uint64_t t, int u, bool d, int pfn, uint64_t pa, bool v);

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
  pair<bool, uint64_t> insert_to_cache(uint64_t dc_index, uint64_t dc_tag, int time, bool dirty, int pfn, uint64_t phys_addr);
  bool check_cache(uint64_t dc_index, uint64_t dc_tag, int time, bool is_write, int pfn , bool check_the_l2_stuff);
  void invalidate_bc_l2_eviction(uint64_t dc_index, uint64_t dc_tag, double& l2_refs, int& memory_refs);
  bool evict_given_pfn(int pfn, int &disk_ref, int &mem_refs, int &page_refs, bool l2_enabled, double &l2_hits);
  bool is_dirty(uint64_t index, uint64_t tag,int pfn);
};

#endif