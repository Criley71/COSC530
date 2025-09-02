#include "dc.h"
using namespace std;

Cache_block::Cache_block(int i, int t, int a){
  index = i;
  tag = t;
  time_last_accessed = a;
}
DC::DC(int sc, int ss, int ls, bool wawb, int ibs, int obs){
  set_count = sc;
  set_size = ss;
  line_size = ls;
  write_allo_write_back = wawb;
  index_bit_size = ibs;
  offset_bit_size = obs;
  data_cache.resize(set_count);//have the outer vector be sets
  for(int i = 0; i < set_count; i++){
    data_cache[i].resize(set_size, Cache_block(-1, -1, -1));//split each set into entries
  }
}

void DC::insert_to_cache(int dec_dc_index, int dec_dc_tag, int time){
  int oldest_used = INT32_MAX;
  int oldest_index = 0;
  for(int i = 0; i < set_size; i++){
    if(data_cache[dec_dc_index][i].time_last_accessed < oldest_used){
      oldest_index = i;
      oldest_used = data_cache[dec_dc_index][i].time_last_accessed;
    }
  }
  data_cache[dec_dc_index][oldest_index].index = dec_dc_index;
  data_cache[dec_dc_index][oldest_index].tag = dec_dc_tag;
  data_cache[dec_dc_index][oldest_index].time_last_accessed = time;
  //cout << " INSERTING " << data_cache[dec_dc_index][oldest_index].tag << " at index: " << dec_dc_index << " with time " << data_cache[dec_dc_index][oldest_index].time_last_accessed;
}

bool DC::check_cache(int dec_dc_index, int dec_dc_tag, int time){
  //cout << "SIZE: " << data_cache.size();
  //cout << "DATA CACHE INDEX 1: " << data_cache[1][0].tag << "\n";
  //cout << " check match " << data_cache[dec_dc_index][0].tag << " ";
  for(int i = 0; i < set_size; i++){
    //cout << " HERE data_dc_tag>" <<data_cache[dec_dc_index][i].tag  << " vs " << dec_dc_tag << "< ";
    
    if(data_cache[dec_dc_index][i].tag == dec_dc_tag ){
      //cout << "Matched " << data_cache[dec_dc_index][i].tag << " with " << dec_dc_tag << " ";
      data_cache[dec_dc_index][i].time_last_accessed = time;
      return true;
    }
  }
  insert_to_cache(dec_dc_index, dec_dc_tag, time);
  //cout << " check insert " << data_cache[dec_dc_index][0].tag << " ";
  return false;
}
