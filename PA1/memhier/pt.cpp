#include "pt.h"

PT::PT(int virtual_page_count) {
  page_table.resize(virtual_page_count);
  pfn_used_count = 0;
}

int PT::check_page_table(int vpn, int timer, bool is_write) {

  if (page_table[vpn].pfn != -1) {
    if (is_write) {
      page_table[vpn].dirty = true;
    }
    page_table[vpn].time = timer;
  }
  return page_table[vpn].pfn; //-1 means not set
}

pair<int, bool> PT::insert_page(int vpn, int vpc, int ppc, int timer, bool is_write, DC &cache, L2 &l2, int &disk_ref, DTLB &dtlb, int &mem_refs, double &l2_refs) {
  int pfn;
  if (ppc == pfn_used_count) {
    bool was_dirty = false;
    int oldest_time = INT32_MAX;
    int oldest_pfn = -1;
    int oldest_index = 0;
    for (int i = 0; i < vpc; i++) {
      if (page_table[i].valid == true) {
        if (page_table[i].time < oldest_time) {
          oldest_time = page_table[i].time;
          oldest_pfn = page_table[i].pfn;
          was_dirty = page_table[i].dirty;
          oldest_index = i;
        }
      }
    }
    if (page_table[oldest_index].dirty) {
      disk_ref += 1;
      // mem_refs += 1;
    }

    pfn = oldest_pfn; // use the evicted pfn for cache invalidation
    // Invalidate any cache/L2 entries that reference this pfn
    bool was_dc_dirty = cache.check_cache(-1, -1, -1, false, pfn, true);
    if(was_dc_dirty){
      //mem_refs+=1;
    }
    // Remove L2 entries that reference this physical frame and evict associated DC entries
    int temp = mem_refs;
    // cout << "pfn looking to be evicted: " << dec << pfn << hex << " ";
    vector<pair<int, int>> dc_pairs = l2.evict_entries_by_pfn(pfn, mem_refs, l2_refs);
    
    // cout << " dc_pairs size " << dc_pairs.size() << " ";
    // For each DC pair, evict from the data cache
    for (int i = 0; i < dc_pairs.size(); i++) {
      int dc_index = dc_pairs[i].first;
      int dc_tag = dc_pairs[i].second;

      bool was_dirty = cache.evict_given_l2_phys_address(dc_index, dc_tag);
      if (was_dirty) {
        //l2_refs+=1;
        // cout << "test";
        //mem_refs += 1;
      }
    }
    // cout<< dec << temp << " after l2 evictions is now " << mem_refs << hex << "   |";
    // cout << "checking pfn " << pfn << " |" << flush;
    int evicted_vpn = oldest_index;
    dtlb.remove_dtlb_entries_bc_pt_eviction(evicted_vpn, pfn);
    // cout << "IS THIS USED\n";
    page_table[oldest_index].dirty = false;
    page_table[oldest_index].time = -1;
    page_table[oldest_index].pfn = -1;
    page_table[oldest_index].valid = false;

    page_table[vpn].dirty = is_write;
    page_table[vpn].time = timer;
    page_table[vpn].pfn = oldest_pfn;
    page_table[vpn].valid = true;
    //disk_ref += 1;
    //mem_refs += 1;
    return {oldest_pfn, was_dirty};
  } else {
    //disk_ref += 1;
    //mem_refs += 1;
    pfn = pfn_used_count;
    page_table[vpn].dirty = is_write;
    page_table[vpn].time = timer;
    page_table[vpn].pfn = pfn;
    page_table[vpn].valid = true;
    pfn_used_count += 1;
  }
  return {pfn, false};
}

uint64_t PT::vpn_to_phys_address(int vpn, int page_offset, int page_offset_bits, int vpc, int ppc, int timer, bool is_write, DC &cache, L2 &l2, int &disk_ref, DTLB &dtlb, int &mem_refs, double &l2_refs) {
  int pfn;
  bool was_dirty_page;
  if (check_page_table(vpn, timer, is_write) == -1) {
    pfn = insert_page(vpn, vpc, ppc, timer, is_write, cache, l2, disk_ref, dtlb, mem_refs, l2_refs).first;
    // was_dirty_page = insert_page(vpn, vpc, ppc, timer, is_write).second; CANT CALL IT SECOND TIME LIKE THIS BRUH
    // dont think this ^ variable is actually necessary, should just add 1 to main mem ref
  } else {
    pfn = check_page_table(vpn, timer, is_write);
  }
  uint64_t physical_address = (static_cast<uint64_t>(pfn) << page_offset_bits) | page_offset;
  return physical_address;
}

int PT::get_current_page_count() {
  return pfn_used_count;
}