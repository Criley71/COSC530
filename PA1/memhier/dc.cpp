#include "dc.h"
using namespace std;

Cache_block::Cache_block(int i, int t, int a, int db, int pf) {
  index = i;
  tag = t;
  time_last_accessed = a;
  dirty_bit = db;
  pfn = pf;
}
DC::DC(int sc, int ss, int ls, bool wawb, int ibs, int obs) {
  set_count = sc;
  set_size = ss;
  line_size = ls;
  write_allo_write_back = wawb;
  index_bit_size = ibs;
  offset_bit_size = obs;
  data_cache.resize(set_count); // have the outer vector be sets

  for (int i = 0; i < set_count; i++) {
    data_cache[i].resize(set_size, Cache_block(-1, -1, -1, -1,-1)); // split each set into entries
  }
}

void DC::insert_to_cache(int dec_dc_index, int dec_dc_tag, int time, int dirty_bit, int pfn) {
  int oldest_used = INT32_MAX;
  int oldest_index = 0;
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dec_dc_index][i].time_last_accessed < oldest_used) {
      oldest_index = i;
      oldest_used = data_cache[dec_dc_index][i].time_last_accessed;
    }
  }
  data_cache[dec_dc_index][oldest_index].index = dec_dc_index;
  data_cache[dec_dc_index][oldest_index].tag = dec_dc_tag;
  data_cache[dec_dc_index][oldest_index].time_last_accessed = time;
  data_cache[dec_dc_index][oldest_index].dirty_bit = dirty_bit;
  data_cache[dec_dc_index][oldest_index].pfn = pfn;
  // cout << " dirty bit: " << data_cache[dec_dc_index][oldest_index].dirty_bit << " ";
  // cout << " INSERTING " << dec_dc_tag << " at index: " << dec_dc_index << " with time " << data_cache[dec_dc_index][oldest_index].time_last_accessed;
}

bool DC::check_cache(int dec_dc_index, int dec_dc_tag, int time, bool is_write,  int  pfn, bool page_fault) {
  // cout << "SIZE: " << data_cache.size();
  // cout << "DATA CACHE INDEX 1: " << data_cache[1][0].tag << "\n";
  //
  if(page_fault){ //need to invalidate the block with pfn because a page fault occured
    for(int i = 0; i < set_count; i++){
      for(int j = 0; j < set_size; i++){
        if(data_cache[i][j].pfn == pfn){   
          data_cache[i][j] = Cache_block(-1,-1,-1,-1,-1);
          return true;
        }
      }
    }
    return false;
  }


  for (int i = 0; i < set_size; i++) {
    if (data_cache[dec_dc_index][i].tag == dec_dc_tag) {
      data_cache[dec_dc_index][i].time_last_accessed = time;
      if (is_write == true) {
        data_cache[dec_dc_index][i].dirty_bit = 1;
      }
      return true;
    }
  }
  if (is_write) {
    insert_to_cache(dec_dc_index, dec_dc_tag, time, 1, pfn); // inserted on a write, set dirty bit to 1
  } else {
    insert_to_cache(dec_dc_index, dec_dc_tag, time, 0, pfn); // inserted on read, set dirty bit to 0
  }
  return false;
}
