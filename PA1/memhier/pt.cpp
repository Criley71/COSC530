#include "pt.h"

PT::PT(int virtual_page_count){
  page_table.resize(virtual_page_count);
  pfn_used_count = 0;
}

int PT::check_page_table(int vpn){
  return page_table[vpn].pfn; //-1 means not set
}

pair<int,bool> PT::insert_page(int vpn, int vpc, int ppc, int timer, bool is_write, DC cache, L2 l2){
  int pfn;
  if(ppc == pfn_used_count){
    bool was_dirty = false;
    int oldest_time = INT32_MAX;
    int oldest_pfn = -1;
    int oldest_index = 0;
    for(int i = 0; i < vpc; i++){
      if(page_table[i].valid == true){
        if(page_table[i].time < oldest_time){
          oldest_time = page_table[i].time;
          oldest_pfn = page_table[i].pfn;
          was_dirty = page_table[i].dirty;
          oldest_index = i;
        }
      }
    }

    page_table[vpn].dirty = is_write;
    page_table[vpn].time = timer;
    page_table[vpn].pfn = oldest_pfn;
    page_table[vpn].valid = true;

    cache.check_cache(-1, -1, -1,false, pfn, true);
    l2.check_l2(-1,-1,-1,-1,pfn, true, -1,-1);
    //cout << "IS THIS USED\n";
    page_table[oldest_index].dirty = false;
    page_table[oldest_index].time = -1;
    page_table[oldest_index].pfn = -1;
    page_table[oldest_index].valid = false;

    return {oldest_pfn, was_dirty};
  }else{
    pfn = pfn_used_count;
    page_table[vpn].dirty = is_write;
    page_table[vpn].time = timer;
    page_table[vpn].pfn = pfn;
    page_table[vpn].valid = true;
    pfn_used_count += 1;
  }
  return{pfn, false};

}

uint64_t PT::vpn_to_phys_address(int vpn, int page_offset, int page_offset_bits, int vpc, int ppc, int timer, bool is_write, DC cache, L2 l2){
  int pfn;
  bool was_dirty_page;
  if(check_page_table(vpn) == -1){
    pfn = insert_page(vpn, vpc, ppc, timer, is_write, cache, l2).first;
    //was_dirty_page = insert_page(vpn, vpc, ppc, timer, is_write).second; CANT CALL IT SECOND TIME LIKE THIS BRUH
    //dont think this ^ variable is actually necessary, should just add 1 to main mem ref
  }else{
    pfn = check_page_table(vpn);
  }
  uint64_t physical_address = (static_cast<uint64_t>(pfn) << page_offset_bits) | page_offset;
  return physical_address;
  
}