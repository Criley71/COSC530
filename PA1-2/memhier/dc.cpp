#include "dc.h"

Cache_Block::Cache_Block(int t, int u, bool d, int p, string pa) {
  tag = t;
  time_last_used = u;
  dirty = d;
  pfn = p;
  physical_address = pa;
}

DC::DC(int sc, int ss, int ls, bool wa, int ibs, int obs) {
  set_count = sc;
  set_size = ss;
  line_size = ls;
  no_write_allo = wa;
  index_bit_size = ibs;
  offset_bit_size = obs;
  data_cache.resize(set_count);
  for (int i = 0; i < set_count; i++) {
    data_cache[i].resize(set_size, Cache_Block(-1, -1, false, -1, ""));
  }
}

pair<bool, string> DC::insert_to_cache(int dc_index, int dc_tag, int time, bool dirty, int pfn, string phys_addr) {
  int oldest_used = INT32_MAX;
  bool old_dirty = false;
  int oldest_index = 0;
  string old_address = "";
  bool is_full = true;
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].tag == -1) {
      is_full = false;
      break;
    }
  }

  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].time_last_used < oldest_used) {
      oldest_index = i;
      oldest_used = data_cache[dc_index][i].time_last_used;
      old_dirty = data_cache[dc_index][i].dirty;
      old_address = data_cache[dc_index][i].physical_address;
    }
  }

  data_cache[dc_index][oldest_index].tag = dc_tag;
  data_cache[dc_index][oldest_index].time_last_used = time;
  data_cache[dc_index][oldest_index].dirty = dirty;
  data_cache[dc_index][oldest_index].pfn = pfn;
  data_cache[dc_index][oldest_index].physical_address = phys_addr;
  if (is_full) {
    if (old_dirty) {
      return {true, old_address}; // replaced a block and it was dirty
    } else {
      return {false, old_address}; // replaced a clean block
    }
  } else {
    return {false, ""}; // did not have to replace a block
  }
}

bool DC::check_cache(int dc_index, int dc_tag, int time, bool is_write, int pfn, bool page_fault) {
  bool page_replace = false;
  if (page_fault) { // need to invalidate the block with pfn because a page fault occured
    for (int i = 0; i < set_count; i++) {
      for (int j = 0; j < set_size; j++) {
        //if(data_cache[0x2d][j].tag == 0xd3dd){
          //cout << "FOUND IT the associated pfn is " << data_cache[0x2d][j].pfn;
        //}
        if (data_cache[i][j].pfn == pfn) {
          data_cache[i][j] = Cache_Block(-1, -1, false, -1, "");
          page_replace = true;
        }
      }
    }
    return page_replace;
  }
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].tag == dc_tag) {
      //if(dc_tag == 0xd3dd && dc_index == 0x2d){
            //cout << "found this " << pfn << " <- ";
         // }
      if (pfn != -1 && data_cache[dc_index][i].pfn != pfn) {
        continue; // treat as miss for this entry
      }
        data_cache[dc_index][i].time_last_used = time;
      if (is_write && !no_write_allo) {
        data_cache[dc_index][i].dirty = true;
      }
      return true;
    }
  }
  return false;
}
void DC::invalidate_bc_l2_eviction(int dc_index, int dc_tag, double &l2_refs, int &memory_refs) {
  for (int i = 0; i < set_size; i++) {
    if (data_cache[dc_index][i].tag == dc_tag && data_cache[dc_index][i].pfn != -1) {
      if (data_cache[dc_index][i].dirty && !no_write_allo) {
        l2_refs += 1; // possibly a hit also on no write allocate
        memory_refs += 1; // write dirty block back to main memory (count it)
      }
      data_cache[dc_index][i] = Cache_Block(-1, -1, false, -1, "");
    }
  }
}

bool DC::evict_given_pfn(int pfn, int &disk_ref, int &mem_refs, int &page_refs, bool l2_enabled, double &l2_hits) {
  bool page_replace = false;
  for (int i = 0; i < set_count; i++) {
    for (int j = 0; j < set_size; j++) {
      if (data_cache[i][j].pfn == pfn) {
        if (data_cache[i][j].dirty) {
          if(l2_enabled){
            l2_hits+=1;
            //mem_refs += 1; // Need to count memory reference even with L2 since block is dirty
          }else if (!l2_enabled && !no_write_allo){
           mem_refs += 1; // this is called when a page fault occurs with no l2 enabled
          }
          // any dirty will write to memory
        }
        data_cache[i][j] = Cache_Block(-1, -1, false, -1, "");
        page_replace = true;
      }
    }
  }
  return page_replace;
}

bool DC::is_dirty(int index, int tag, int pfn) {
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
