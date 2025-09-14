#include "l2.h"
using namespace std;
// pretty much the same set up as the dc
L2_block::L2_block(int i, int t, int a, int db, int pf, int dct, int dci, string dca) {
  index = i;
  tag = t;
  time_last_accessed = a;
  dirty_bit = db;
  pfn = pf;
  dc_tag = dct;
  dc_index = dci;
  dc_address = dca;
}

L2::L2(int sc, int ss, int ls, bool wawb, int ibs, int obs) {
  set_count = sc;
  set_size = ss;
  line_size = ls;
  write_allo_write_back = wawb;
  index_bit_size = ibs;
  offset_bit_size = obs;

  l2_cache.resize(set_count); // have the outer vector be sets
  for (int i = 0; i < set_count; i++) {
    l2_cache[i].resize(set_size, L2_block(-1, -1, -1, -1, -1, -1,-1,""));
  }
}

pair<bool,string> L2::insert_to_l2(int l2_index, int l2_tag, int time, int dirty_bit, int pfn, int dc_index, int dc_tag, string dc_address, int &counter) {
  bool was_full = check_if_index_is_full(l2_index);
  int oldest_used = INT32_MAX;
  int oldest_index = 0;
  int old_dirty = 0;
  for (int i = 0; i < set_size; i++) {
    if(l2_index == 0x8){
      //cout << " l2_index " << hex << l2_index << " with tag " << hex << l2_cache[l2_index][i].tag << " and time = " << dec <<l2_cache[l2_index][i].time_last_accessed << "|  " ; 
    }
    if (l2_cache[l2_index][i].time_last_accessed < oldest_used) {
      oldest_index = i;
      oldest_used = l2_cache[l2_index][i].time_last_accessed;
      old_dirty = l2_cache[l2_index][i].dirty_bit;
    }
  }
  if(oldest_used != -1 && l2_index == 0x8){
   // cout << "Replaceing " << " l2_index " << hex << l2_index << " with tag " << hex << l2_cache[l2_index][oldest_index].tag << " and time = " << dec <<l2_cache[l2_index][oldest_index].time_last_accessed << "|  ";
  }
  if(old_dirty == 1){
    counter += 1;
  }
  string old_dc_address = l2_cache[l2_index][oldest_index].dc_address;
  l2_cache[l2_index][oldest_index].index = l2_index;
  l2_cache[l2_index][oldest_index].tag = l2_tag;
  l2_cache[l2_index][oldest_index].time_last_accessed = time;
  l2_cache[l2_index][oldest_index].dirty_bit = dirty_bit;
  l2_cache[l2_index][oldest_index].pfn = pfn;
  l2_cache[l2_index][oldest_index].dc_index = dc_index;
  l2_cache[l2_index][oldest_index].dc_tag = dc_tag;
  l2_cache[l2_index][oldest_index].dc_address = dc_address;
  return {was_full, old_dc_address};

  
}

bool L2::check_l2(int l2_index, int l2_tag, int time, int dirty_bit, int pfn, bool page_fault, int dc_index, int dc_tag) {
  if (page_fault) {
    for (int i = 0; i < set_count; i++) {
      for (int j = 0; j < set_size; j++) {
        if (l2_cache[i][j].pfn == pfn) {
          l2_cache[i][j] = L2_block(-1, -1, -1, -1, -1,-1,-1,"");
          return true;
        }
      }
    }
    return false;
  }
  bool was_found = false;
  for (int i = 0; i < set_size; i++) {
    if (l2_cache[l2_index][i].tag == l2_tag) {
      l2_cache[l2_index][i].time_last_accessed = time;
      was_found = true;
    }
  }
  return was_found;
  //insert_to_l2(l2_index, l2_tag, time, dirty_bit, pfn, dc_index, dc_tag);
  return false;
}

bool L2::check_if_index_is_full(int index){
  for(int i = 0; i < set_size; i++){
    if(l2_cache[index][i].tag == -1){
      return false;
    }
  }
  return true;
}

uint64_t L2::l2_index_and_tag_evicted_phys_address(int index){
  if(!check_if_index_is_full(index)){
    return -1;
  }
  int oldest_used = INT32_MAX;
  int oldest_index = -1;
  int l2_tag = 0;
  for (int i = 0; i < set_size; i++) {
    if (l2_cache[index][i].time_last_accessed < oldest_used) {
      oldest_index = i;
      oldest_used = l2_cache[index][i].time_last_accessed;
      l2_tag = l2_cache[index][i].tag;
    }
  }
  cout << "    invalidate index " << hex << index << " with tag " << hex << l2_tag << "    "; 
  uint64_t phys_base = (l2_tag << (index_bit_size + offset_bit_size)) | (index << offset_bit_size);
  return phys_base;
}

pair<int, int> L2::get_dc_index_tag(int l2_index, int l2_tag){

  int oldest_time = INT32_MAX;
  int dc_index;
  int dc_tag;
  int ltag;
  int lndex;
  for(int i = 0; i < set_size; i++){
    cout << hex << " l2tag: "<< l2_cache[l2_index][i].tag<< dec << " counter " << l2_cache[l2_index][i].time_last_accessed << ", ";
    if(l2_cache[l2_index][i].time_last_accessed < oldest_time && l2_cache[l2_index][i].dc_tag!= -1 ){
      dc_index = l2_cache[l2_index][i].dc_index;
      dc_tag = l2_cache[l2_index][i].dc_tag;
      oldest_time = l2_cache[l2_index][i].time_last_accessed;
      lndex=  l2_cache[l2_index][i].index;
      ltag = l2_cache[l2_index][i].tag;
    }
  }
    cout << "      evict l2 cache tag " << hex << ltag << " at index " << hex << lndex << " "; 

  cout << "      evict data cache tag " << hex << dc_tag << " at index " << hex << dc_index << "     "; 
  return {dc_index, dc_tag};
}

void L2::update_access_time(int l2_index, int l2_tag, int time){
  for(int i = 0; i < set_size; i++){
    if(l2_cache[l2_index][i].tag == l2_tag){
      l2_cache[l2_index][i].time_last_accessed = time;
      return;
    }
  }
}

void L2::update_dirty_bit(int l2_index, int l2_tag, int time){
   for(int i = 0; i < set_size; i++){
    if(l2_cache[l2_index][i].tag == l2_tag){
      //cout << " updating dirty bit of index:" << hex << l2_index << " tag: " << l2_tag << "|";
      //if(l2_index != 0xd){
        //l2_cache[l2_index][i].time_last_accessed = time;
      //}
      l2_cache[l2_index][i].dirty_bit = 1;
      return;
    }
  }
}
