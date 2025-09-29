#include "l2.h"
using namespace std;
L2_block::L2_block(int i, int t, int a, bool d, int pf, int dcr, bool v) {
  index = i;
  tag = t;
  time_last_used = a;
  dirty = d;
  pfn = pf;
  dc_ratio = dcr;
  valid =v;
  dc_index_and_tags.resize(dc_ratio, {-1, -1});
}

L2::L2(int sc, int ss, int ls, bool wa, int ibs, int obs, int dcr) {
  set_count = sc;
  set_size = ss;
  line_size = ls;
  write_allo_write_back = wa;
  index_bit_size = ibs;
  offset_bit_size = obs;
  dc_ratio = dcr;
  l2_cache.resize(set_count);
  for (int i = 0; i < set_count; i++) {
    l2_cache[i].resize(set_size, L2_block(-1, -1, -1, false, -1, dc_ratio, false));
  }
}

pair<bool, vector<pair<int, int>>> L2::insert_to_l2(int l2_index, int l2_tag, int time, bool dirty, int pfn, int &memory_refs, int dc_i, int dc_t) {
  bool was_full = check_if_index_is_full(l2_index);
  int oldest_used = INT32_MAX;
  int oldest_index = 0;
  bool old_dirty = 0;
  for (int i = 0; i < set_size; i++) {
    if (l2_index == 0xb) {
      // cout << " l2_index " << hex << l2_index << " with tag " << hex << l2_cache[l2_index][i].tag << " and time = " << dec <<l2_cache[l2_index][i].time_last_accessed << "|  " ;
    }
    if (l2_cache[l2_index][i].time_last_used < oldest_used) {
      oldest_index = i;
      oldest_used = l2_cache[l2_index][i].time_last_used;
      old_dirty = l2_cache[l2_index][i].dirty;
    }
  }
  if (oldest_used != -1 && l2_index == 0xb) {
    // cout << "  Replaceing " << " l2_index " << hex << l2_index << " with tag " << hex << l2_cache[l2_index][oldest_index].tag << " and time = " << dec <<l2_cache[l2_index][oldest_index].time_last_accessed << "|  ";
  }
  if (old_dirty == 1) {
    memory_refs += 1; //if the old block was dirty then write to memory
  }
  vector<pair<int, int>> old_dc_address = l2_cache[l2_index][oldest_index].dc_index_and_tags;
  l2_cache[l2_index][oldest_index].index = l2_index;
  l2_cache[l2_index][oldest_index].tag = l2_tag;
  l2_cache[l2_index][oldest_index].time_last_used = time;
  l2_cache[l2_index][oldest_index].dirty = dirty;
  l2_cache[l2_index][oldest_index].pfn = pfn;
  l2_cache[l2_index][oldest_index].dc_index_and_tags.resize(dc_ratio, {-1, -1});
  l2_cache[l2_index][oldest_index].dc_index_and_tags[0] = {dc_i, dc_t};
  l2_cache[l2_index][oldest_index].valid = true;
  return {was_full, old_dc_address};
}

void L2::insert_address_to_block(int index, int tag, int dc_i, int dc_t) {
  for (int j = 0; j < set_size; j++) {
    if (l2_cache[index][j].tag == tag) {
      for (int i = 0; i < dc_ratio; i++) {
        if (l2_cache[index][j].dc_index_and_tags[i].first == -1) {
          l2_cache[index][j].dc_index_and_tags[i].first = dc_i;
          l2_cache[index][j].dc_index_and_tags[i].second = dc_t;
        }
      }
      return;
    }
  }
}

bool L2::check_l2(int l2_index, int l2_tag, int time, bool dirty, int pfn, bool page_fault) {
  bool page_replace = false;
  if (page_fault) {
    for (int i = 0; i < set_count; i++) {
      for (int j = 0; j < set_size; j++) {
        if (l2_cache[i][j].pfn == pfn) {
          l2_cache[i][j] = L2_block(-1, -1, -1, false, -1, dc_ratio, false);
          page_replace = true;
        }
      }
    }
    return page_replace;
  }
  bool was_found = false;
  for (int i = 0; i < set_size; i++) {
    if (l2_cache[l2_index][i].tag == l2_tag) {
      l2_cache[l2_index][i].time_last_used = time;
      return true; // CHANGED THIS MAKE SURE IT WORKS STILL USED TO BE WAS_FOUND
    }
  }
  // return was_found;

  return false;
}

bool L2::check_if_index_is_full(int index) {
  for (int i = 0; i < set_size; i++) {
    if (l2_cache[index][i].tag == -1) {
      return false;
    }
  }
  return true;
}

void L2::update_used_time(int l2_index, int l2_tag, int timer) {
  for (int i = 0; i < set_size; i++) {
    if (l2_cache[l2_index][i].tag == l2_tag) {
      l2_cache[l2_index][i].time_last_used = timer;
      return;
    }
  }
}

void L2::update_dirty_bit(int l2_index, int l2_tag, int timer) {
  for (int i = 0; i < set_size; i++) {
    if (l2_cache[l2_index][i].tag == l2_tag) {
      l2_cache[l2_index][i].time_last_used = timer;
      l2_cache[l2_index][i].dirty = true;
      return;
    }
  }
}

pair<bool, vector<pair<int,int>>> L2::evict_given_pfn(int pfn, int &memory_refs, double &l2_hits, DC &dc){
  vector<pair<int,int>> evicted_dc_blocks = {};
  int amount_of_dirty = 0;
  bool replace = false;
  for(int i = 0; i < set_count; i ++){
    for(int j = 0; j < set_size; j++){
      if(l2_cache[i][j].pfn == pfn && l2_cache[i][j].valid){
        for(int k = 0; k < l2_cache[i][j].dc_index_and_tags.size(); k++){
          if(l2_cache[i][j].dc_index_and_tags[k].first != -1){
            evicted_dc_blocks.push_back({l2_cache[i][j].dc_index_and_tags[k].first, l2_cache[i][j].dc_index_and_tags[k].second});
            if(dc.is_dirty(l2_cache[i][j].dc_index_and_tags[k].first, l2_cache[i][j].dc_index_and_tags[k].second, pfn)){
              amount_of_dirty+=1;
            }
          }
        }
        // we need to avoid double counting memory refs if both the dc and l2 were dirty, 
        // this handles the case of l2 being larger than dc
        if (l2_cache[i][j].dirty) {
          if (amount_of_dirty == 0) {
            memory_refs += 1; // write the dirty L2 block
          } else if (amount_of_dirty > 1 && dc_ratio == 1) {
            memory_refs += (amount_of_dirty - 1); // avoid double-count
          } else { // amount_of_dirty == 1
            memory_refs += 1;
          }
        } else {
          memory_refs += amount_of_dirty;
        }
        amount_of_dirty = 0;
        l2_cache[i][j] = L2_block(-1, -1, -1, false, -1, dc_ratio, false);
        replace = true;
      } 
    }
  }
  
  return {replace, evicted_dc_blocks};
} 
