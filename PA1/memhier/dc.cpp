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

void DC::insert_to_cache(int dc_index, int dc_tag, int time, int dirty_bit, int pfn) {
  int oldest_used = INT32_MAX;
  int old_dirty_bit = -1;
  int oldest_index = 0;
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].time_last_accessed < oldest_used) {
      oldest_index = i;
      oldest_used = data_cache[dc_index][i].time_last_accessed;
      old_dirty_bit = data_cache[dc_index][i].dirty_bit;
    }
  }
  data_cache[dc_index][oldest_index].index = dc_index;
  data_cache[dc_index][oldest_index].tag = dc_tag;
  data_cache[dc_index][oldest_index].time_last_accessed = time;
  data_cache[dc_index][oldest_index].dirty_bit = dirty_bit;
  data_cache[dc_index][oldest_index].pfn = pfn;
  if(old_dirty_bit != -1){
    if(old_dirty_bit = 1){
      //return true; //TODO: UPDATE THE INSERT TO RETURN BOOL IF REPLACED WAS DIRTY
      // PROBABLY ALSO NEED TO RETURN THE REPLACED BLOCKS PHYSICAL ADDRESS TO THEN CALCULATE L2 ADDRESS
      // THIS WILL REQUIRE THE CACHE BLOCK STRUCTURE TO HOLD THE PHYS ADDRESS AS A STRING LIKE THE L2
    }else{
      //return false;
    }
  }
  // cout << " dirty bit: " << data_cache[dc_index][oldest_index].dirty_bit << " ";
  // cout << " INSERTING " << dc_tag << " at index: " << dc_index << " with time " << data_cache[dc_index][oldest_index].time_last_accessed;
}

bool DC::check_cache(int dc_index, int dc_tag, int time, bool is_write,  int  pfn, bool page_fault) {
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
    if (data_cache[dc_index][i].tag == dc_tag) {
      data_cache[dc_index][i].time_last_accessed = time;
      if (is_write == true) {
        data_cache[dc_index][i].dirty_bit = 1;
      }
      return true;
    }
  }
  if (is_write) {
    insert_to_cache(dc_index, dc_tag, time, 1, pfn); // inserted on a write, set dirty bit to 1
  } else {
    insert_to_cache(dc_index, dc_tag, time, 0, pfn); // inserted on read, set dirty bit to 0
  }
  return false;
}



void DC::evict_given_l2_phys_address(int dc_index, int dc_tag){
  for(int i = 0; i < set_size; i++){
    if(data_cache[dc_index][i].tag == dc_tag){
     // int dc_index_check = 0x1e8; 
      //if(dc_index == dc_index_check){
        //cout << "!!!!!!!!!!!!!EVICTING!!!!!!!!!!!!!!!!!!!";
      //}
      data_cache[dc_index][i] = Cache_block(-1,-1,-1,-1,-1);
      return;
    }
  }
}
