#include "l2.h"
using namespace std;
L2_block::L2_block(uint64_t i, uint64_t t, int a, bool d, int pf, int dcr, bool v, uint64_t add) {
  index = i;
  tag = t;
  time_last_used = a;
  dirty = d;
  pfn = pf;
  dc_ratio = dcr;
  valid = v;
  address = add;
  dc_index_and_tags.resize(dc_ratio, {0, 0});
}

L2::L2(int sc, int ss, int ls, bool wa, int ibs, int obs, int dcr) {
  set_count = sc;
  set_size = ss;
  line_size = ls;
  no_write_allo = wa;
  index_bit_size = ibs;
  offset_bit_size = obs;
  dc_ratio = dcr;
  l2_cache.resize(set_count);
  for (int i = 0; i < set_count; i++) {
    l2_cache[i].resize(set_size, L2_block(0, 0, -1, false, -1, dc_ratio, false, 0));
  }
}

pair<bool, vector<pair<uint64_t, uint64_t>>> L2::insert_to_l2(uint64_t l2_index, uint64_t l2_tag, int time, bool dirty, int pfn, int &memory_refs, uint64_t dc_i, uint64_t dc_t) {
  bool was_full = check_if_index_is_full(l2_index);
  int oldest_used = INT32_MAX;
  int oldest_entry = 0;
  bool old_dirty = 0;
  for (int i = 0; i < set_size; i++) {

    if (l2_cache[l2_index][i].time_last_used < oldest_used || !l2_cache[l2_index][i].valid) {
      oldest_entry = i;
      oldest_used = l2_cache[l2_index][i].time_last_used;
      old_dirty = l2_cache[l2_index][i].dirty;
      if(!l2_cache[l2_index][i].valid){
        break;
      }
    }
  }
  
    
    //for (int i = 0; i < set_size; i++) {
      //cout << "l2 index " << l2_index << " with tag " << l2_cache[l2_index][i].tag << " has a time of " << dec << l2_cache[l2_index][i].time_last_used << " | " << hex;
    //}
  
  if (old_dirty == 1) {
    memory_refs += 1; // if the old block was dirty then write to memory
  }
  //if (oldest_used != -1) {
     //cout << hex << " chose: " << l2_cache[l2_index][oldest_entry].index << " with tag " << l2_cache[l2_index][oldest_entry].tag << " | ";
  //}
  vector<pair<uint64_t, uint64_t>> old_dc_address = l2_cache[l2_index][oldest_entry].dc_index_and_tags;
  l2_cache[l2_index][oldest_entry].index = l2_index;
  l2_cache[l2_index][oldest_entry].tag = l2_tag;
  l2_cache[l2_index][oldest_entry].time_last_used = time;
  l2_cache[l2_index][oldest_entry].dirty = dirty;
  l2_cache[l2_index][oldest_entry].pfn = pfn;
  l2_cache[l2_index][oldest_entry].dc_index_and_tags.resize(dc_ratio, {0, 0});
  l2_cache[l2_index][oldest_entry].dc_index_and_tags[0] = {dc_i, dc_t};
  l2_cache[l2_index][oldest_entry].valid = true;
  return {was_full, old_dc_address};
}

void L2::insert_address_to_block(uint64_t index, uint64_t tag, uint64_t dc_i, uint64_t dc_t, DC &cache, int timer, int map_i) {
    for (int j = 0; j < set_size; j++) {
      if (l2_cache[index][j].tag == tag && l2_cache[index][j].valid) {
        l2_cache[index][j].dc_index_and_tags[map_i] = {dc_i, dc_t};
        return;
      }
    }
  
}

  bool L2::check_l2(uint64_t l2_index, uint64_t l2_tag, int time, bool dirty, int pfn, bool page_fault) {
    bool page_replace = false;
    if (page_fault) {
      for (int i = 0; i < set_count; i++) {
        for (int j = 0; j < set_size; j++) {
          if (l2_cache[i][j].pfn == pfn && l2_cache[i][j].valid) {

            l2_cache[i][j] = L2_block(0, 0, -1, false, -1, dc_ratio, false, 0);
            page_replace = true;
          }
        }
      }
      return page_replace;
    }
    bool was_found = false;
    for (int i = 0; i < set_size; i++) {
      if (l2_cache[l2_index][i].tag == l2_tag && l2_cache[l2_index][i].valid) {
        if (dirty) {
          l2_cache[l2_index][i].dirty = true;
        }
        l2_cache[l2_index][i].time_last_used = time;
        return true; // CHANGED THIS MAKE SURE IT WORKS STILL USED TO BE WAS_FOUND
      }
    }
    // return was_found;

    return false;
  }

  bool L2::check_if_index_is_full(int index) {
    for (int i = 0; i < set_size; i++) {
      if (!l2_cache[index][i].valid) {
        return false;
      }
    }
    return true;
  }

  void L2::update_used_time(uint64_t l2_index, uint64_t l2_tag, int timer) {
    for (int i = 0; i < set_size; i++) {
      if (l2_cache[l2_index][i].tag == l2_tag) {
        l2_cache[l2_index][i].time_last_used = timer;
        return;
      }
    }
  }

  void L2::update_dirty_bit(uint64_t l2_index, uint64_t l2_tag, int timer) {
    for (int i = 0; i < set_size; i++) {
      if (l2_cache[l2_index][i].tag == l2_tag && l2_cache[l2_index][i].valid) {
          l2_cache[l2_index][i].time_last_used = timer;
        l2_cache[l2_index][i].dirty = true;
        return;
      }
    }
  }

  pair<bool, vector<pair<uint64_t, uint64_t>>> L2::evict_given_pfn(int pfn, int &memory_refs, double &l2_hits, DC &dc) {
    vector<pair<uint64_t, uint64_t>> evicted_dc_blocks = {};
    int amount_of_dirty = 0;
    bool replace = false;
    for (int i = 0; i < set_count; i++) {
      for (int j = 0; j < set_size; j++) {
        if (l2_cache[i][j].pfn == pfn && l2_cache[i][j].valid) {
          //cout << " evicting l2 index " << l2_cache[i][j].index << " tag " << l2_cache[i][j].tag << " ";
          for (int k = 0; k < l2_cache[i][j].dc_index_and_tags.size(); k++) {
            //if (l2_cache[i][j].dc_index_and_tags[k].first != 0 && l2_cache[i][j].dc_index_and_tags[k].second != 0) {
              //cout << "This has dc blocks index " << l2_cache[i][j].dc_index_and_tags[k].first << " tag " << l2_cache[i][j].dc_index_and_tags[k].second << "  ";
              evicted_dc_blocks.push_back({l2_cache[i][j].dc_index_and_tags[k].first, l2_cache[i][j].dc_index_and_tags[k].second});
              if (dc.is_dirty(l2_cache[i][j].dc_index_and_tags[k].first, l2_cache[i][j].dc_index_and_tags[k].second, pfn)) {
                amount_of_dirty += 1;
              }
            //}
          }
          // we need to avoid double counting memory refs if both the dc and l2 were dirty,
          // this handles the case of l2 being larger than dc
          if (l2_cache[i][j].dirty) {
            if (amount_of_dirty == 0) {
              memory_refs += 1; // write the dirty L2 block
            } else if (amount_of_dirty > 1 && dc_ratio == 1) {
              memory_refs += (amount_of_dirty - 1); // avoid double-count
            } else {                                // amount_of_dirty == 1
              memory_refs += 1;
            }
          } else {
            memory_refs += amount_of_dirty;
          }
          amount_of_dirty = 0;
          //cout << " invalidating l2 index " << i << " tag " << l2_cache[i][j].tag << " due to page replacement ";
          l2_cache[i][j] = L2_block(0, 0, -1, false, -1, dc_ratio, false, 0);
          replace = true;
        }
      }
    }

    return {replace, evicted_dc_blocks};
  }

  void L2::update_the_dc_ind_tag(uint64_t l2_index, uint64_t l2_tag, uint64_t dc_index, uint64_t dc_tag, DC &cache, int pfn, int &timer) {
    for (int i = 0; i < set_size; i++) {
      if (l2_cache[l2_index][i].tag == l2_tag) {
        for (int j = 0; j < dc_ratio; j++) {
          if (l2_cache[l2_index][i].dc_index_and_tags[j].first == 0 || !cache.check_cache(dc_index, dc_tag, timer, false, pfn, true)) {
            l2_cache[l2_index][i].dc_index_and_tags[j] = {dc_index, dc_tag};
          }
        }
      }
    }
  }
