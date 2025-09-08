#include "l2.h"
using namespace std;
//pretty much the same set up as the dc
L2_block::L2_block(int i, int t, int a, int db, int pf){
  index = i;
  tag = t;
  time_last_accessed = a;
  dirty_bit = db;
  pfn = pf;
}

L2::L2(int sc, int ss, int ls, bool wawb, int ibs, int obs){
  set_count = sc;
  set_size = ss;
  line_size = ls;
  write_allo_write_back = wawb;
  index_bit_size = ibs;
  offset_bit_size = obs;

  l2_cache.resize(set_count); // have the outer vector be sets
  for(int i = 0; i < set_count; i++){
    l2_cache[i].resize(set_size, L2_block(-1, -1, -1, -1,-1));
  }
}

void L2::insert_to_l2(int dec_l2_index, int dec_l2_tag, int time, int dirty_bit, int pfn){
  int oldest_used = INT32_MAX;
  int oldest_index = 0;
  for(int i = 0; i < set_size; i++){
    if(l2_cache[dec_l2_index][i].time_last_accessed < oldest_used){
      oldest_index = i;
      oldest_used = l2_cache[dec_l2_index][i].time_last_accessed;
    }
  }
  l2_cache[dec_l2_index][oldest_index].index = dec_l2_index;
  l2_cache[dec_l2_index][oldest_index].tag = dec_l2_tag;
  l2_cache[dec_l2_index][oldest_index].time_last_accessed = time;
  l2_cache[dec_l2_index][oldest_index].dirty_bit = dirty_bit;
  l2_cache[dec_l2_index][oldest_index].pfn = pfn;
  
}

bool L2::check_l2(int dec_l2_index, int dec_l2_tag, int time, int dirty_bit, int pfn, bool page_fault){
  if(page_fault){
    for(int i = 0; i < set_count; i++){
      for(int j = 0; j < set_size; j++){
        if(l2_cache[i][j].pfn == pfn){
          l2_cache[i][j] = L2_block(-1,-1,-1,-1,-1);
          return true;
        }
      }
    }
    return false;
  }

  for(int i = 0; i < set_size; i++){
    if(l2_cache[dec_l2_index][i].tag == dec_l2_tag){
      l2_cache[dec_l2_index][i].time_last_accessed = time;
      return true;
    }
  }
  insert_to_l2(dec_l2_index, dec_l2_tag, time, dirty_bit,pfn);
  return false;
}