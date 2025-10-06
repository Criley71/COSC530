#include "pt.h"
using namespace std;
Page::Page(int pt_l2_ratio) {
  //l2_index_tag_map.resize(pt_l2_ratio, {-1, -1});
}

PT::PT(int virtual_page_count, int pt_l2_ratio, int physical_page_count) {
  page_table.resize(virtual_page_count, Page(pt_l2_ratio));
  pfn_in_use_count = 0;
  vpns_in_use.resize(physical_page_count, -1);
}
//check for pfn and return it
int PT::check_page_table(int vpn, int timer, bool is_write) { // return pfn, -1 means not found
  if (page_table[vpn].pfn != -1) {
    if (is_write) {
      page_table[vpn].dirty = true;
    }
    page_table[vpn].time_last_used = timer;
  }
  return page_table[vpn].pfn;
}
//insert page
pair<bool, int> PT::insert_page(int vpn, int vpc, int ppc, int timer, bool is_write, int page_offset, int page_offset_bits, bool l2_enabled, DC &cache, L2 &l2, int &disk_ref, int &mem_ref, double &l2_ref, int& page_refs, bool dtlb_enabled, DTLB &dtlb){
  int pfn;
  if (ppc == pfn_in_use_count) { // if all pfns are in use then we need to evict
    bool was_dirty = false;
    int oldest_time = INT32_MAX;
    int oldest_pfn = -1;
    int oldest_index = 0;
    //cout << "test";
    
      for (int i = 0; i < vpns_in_use.size(); i++) {
        int vpn_in_slot = vpns_in_use[i];
        // skip empty slots
        if (vpn_in_slot == -1) continue;
        if (vpn_in_slot < 0 || vpn_in_slot >= (int)page_table.size()) continue;
        if (page_table[vpn_in_slot].time_last_used < oldest_time && page_table[vpn_in_slot].valid && page_table[vpn_in_slot].pfn != -1) {
          oldest_index = i;
          oldest_pfn = page_table[vpn_in_slot].pfn;
          oldest_time = page_table[vpn_in_slot].time_last_used;
          was_dirty = page_table[vpn_in_slot].dirty;
        }
      }
    pfn = oldest_pfn;
    
    if(was_dirty){
      disk_ref+=1;
    }
    if(!l2_enabled){
      cache.evict_given_pfn(pfn, disk_ref, mem_ref, page_refs, l2_enabled, l2_ref);
    }else{ //evicting a page leads to needing to invalidate dc and l2
      int temp_mem_ref = mem_ref;
     // cout << " physical page " << pfn << " is being replaced";
      pair<bool, vector<pair<uint64_t,uint64_t>>> dc_to_evict = l2.evict_given_pfn(pfn, temp_mem_ref, l2_ref, cache);
      
      if(dc_to_evict.first){
        //cout << "test || Evicting page VPN " << hex << vpns_in_use[oldest_index] << " from PFN " << hex << pfn << " || "; 
        for(int i = 0; i < dc_to_evict.second.size(); i++){
            cache.invalidate_bc_l2_eviction(dc_to_evict.second[i].first, dc_to_evict.second[i].second, l2_ref, mem_ref);
        }
      }
      cache.evict_given_pfn(pfn, disk_ref, temp_mem_ref, page_refs, l2_enabled, l2_ref);
      if(!l2.no_write_allo && !cache.no_write_allo){
        mem_ref = temp_mem_ref;
      }else if(l2.no_write_allo && !cache.no_write_allo){
        mem_ref = mem_ref;
      }else if(!l2.no_write_allo && cache.no_write_allo){
        mem_ref = temp_mem_ref;
      }
      //temp_mem_refs = mem_ref;
      
    }
    if(dtlb_enabled){
      dtlb.remove_dtlb_entries_bc_pt_eviction(vpn, pfn);
    }
    
    page_table[vpns_in_use[oldest_index]].dirty = false;
    page_table[vpns_in_use[oldest_index]].pfn = -1;
    page_table[vpns_in_use[oldest_index]].time_last_used = -1;
    page_table[vpns_in_use[oldest_index]].valid = false;
    vpns_in_use[oldest_index] = vpn;
    page_table[vpn].dirty = is_write;
    page_table[vpn].pfn = pfn;
    page_table[vpn].time_last_used = timer;
    page_table[vpn].valid = 1;
    
    return{was_dirty, pfn};
  } else {
    pfn = pfn_in_use_count;
    page_table[vpn].dirty = is_write;
    page_table[vpn].time_last_used = timer;
    page_table[vpn].pfn = pfn;
    page_table[vpn].valid = true;
    // record that this PFN is now used by this VPN
    if (pfn >= 0 && pfn < (int)vpns_in_use.size()) {
      vpns_in_use[pfn] = vpn;
    }
    pfn_in_use_count+=1;
  }
  //cout << "pfn from pt " << pfn << " | ";
  return {false, pfn};
}
//translates pfn to physical address
uint64_t PT::translate_to_phys_address(uint64_t pfn, int page_offset, int page_offset_bits) {
  uint64_t physical_address = (pfn << page_offset_bits) | page_offset;
  return physical_address;
}