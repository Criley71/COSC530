#include "dc.h"

Cache_Block::Cache_Block(uint64_t t, int u, bool d, int p, uint64_t pa, bool v) {
  tag = t;
  time_last_used = u;
  dirty = d;
  pfn = p;
  physical_address = pa;
  valid = v;
}

DC::DC(int sc, int ss, int ls, bool wa, int ibs, int obs) {
  set_count = sc;
  set_size = ss;
  line_size = ls;
  no_write_allo = wa;
  index_bit_size = ibs;
  offset_bit_size = obs;
  data_cache.resize(set_count);
  for (int i = 0; i < set_count; i++) { // 2d vector of cache block objects
    data_cache[i].resize(set_size, Cache_Block(0, -1, false, -1, 0, false));
  }
}

pair<bool, uint64_t> DC::insert_to_cache(uint64_t dc_index, uint64_t dc_tag, int time, bool dirty, int pfn, uint64_t phys_addr) {
  int oldest_used = INT32_MAX;
  bool old_dirty = false;
  int oldest_index = 0;
  uint64_t old_address = 0;
  bool is_full = true;
  // did this before i made the valid flag and i dont want to change for fear of breaking it all
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].time_last_used == -1) {
      is_full = false;
      break;
    }
  }
  // finds least recently used
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].time_last_used < oldest_used) {
      oldest_index = i;
      oldest_used = data_cache[dc_index][i].time_last_used;
      old_dirty = data_cache[dc_index][i].dirty;
      old_address = data_cache[dc_index][i].physical_address;
    }
  }
  // overwrite the block with new info
  data_cache[dc_index][oldest_index].tag = dc_tag;
  data_cache[dc_index][oldest_index].time_last_used = time;
  data_cache[dc_index][oldest_index].dirty = dirty;
  data_cache[dc_index][oldest_index].pfn = pfn;
  data_cache[dc_index][oldest_index].physical_address = phys_addr;
  data_cache[dc_index][oldest_index].valid = true;
  if (is_full) {
    if (old_dirty) {
      return {true, old_address}; // replaced a block and it was dirty
    } else {
      return {false, old_address}; // replaced a clean block
    }
  } else {
    return {false, 0}; // did not have to replace a block
  }
}

bool DC::check_cache(uint64_t dc_index, uint64_t dc_tag, int time, bool is_write, int pfn, bool check_the_l2_stuff) {
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].tag == dc_tag && data_cache[dc_index][i].valid) {
      if (!check_the_l2_stuff) { // to not update the block use time when checking sometimes
        data_cache[dc_index][i].time_last_used = time;
      }
      if (is_write && !no_write_allo) {
        data_cache[dc_index][i].dirty = true;
      }
      return true; // was found
    }
  }
  return false; // wasnt found
}
//special function for evicitng dc when l2 is evicted
void DC::invalidate_bc_l2_eviction(uint64_t dc_index, uint64_t dc_tag, double &l2_refs, int &memory_refs) {
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].tag == dc_tag && data_cache[dc_index][i].valid) {
      if (data_cache[dc_index][i].dirty) {
        l2_refs += 1;     
        memory_refs += 1; // write dirty block back to main memory (count it)
      }
      data_cache[dc_index][i] = Cache_Block(0, -1, false, -1, 0, false);
    }
  }
}
//for when no l2 is enabled so we just need to invalidate based on pfn
bool DC::evict_given_pfn(int pfn, int &disk_ref, int &mem_refs, int &page_refs, bool l2_enabled, double &l2_hits) {
  bool page_replace = false;
  for (int i = 0; i < set_count; i++) {
    for (int j = 0; j < set_size; j++) {
      if (data_cache[i][j].pfn == pfn && data_cache[i][j].valid) {
        // cout << " | evicting dc index " <<  i << " tag " << data_cache[i][j].tag << " | ";
        if (data_cache[i][j].dirty) {
          if (l2_enabled) {
            l2_hits += 1;
            // mem_refs += 1; // Need to count memory reference even with L2 since block is dirty
          } else if (!l2_enabled && !no_write_allo) {
            mem_refs += 1; // this is called when a page fault occurs with no l2 enabled
          }
          // any dirty will write to memory
        }
        data_cache[i][j] = Cache_Block(0, -1, false, -1, 0, false);
        page_replace = true;
      }
    }
  }
  return page_replace;
}
//returns if the block was dirty
bool DC::is_dirty(uint64_t index, uint64_t tag, int pfn) {
  for (int i = 0; i < set_size; i++) {
    if (data_cache[index][i].tag == tag) {
      if (pfn != -1 && data_cache[index][i].pfn != pfn) {
        continue;
      } else {
        if (data_cache[index][i].dirty) {
          return true;
        } else {
          return false;
        }
      }
    }
  }
  return false;
}
