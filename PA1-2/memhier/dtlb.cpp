#include "dtlb.h"
using namespace std;

DTLB_entry::DTLB_entry(int t, int p) {
  tag = t;
  pfn = p;
  time_last_used = -1;
  valid = false;
}

DTLB::DTLB(int sc, int ss) {
  set_count = sc;
  set_size = ss;
  dtlb.resize(set_count);
  for (int i = 0; i < set_count; i++) {
    dtlb[i].resize(set_size, DTLB_entry(-1, -1));
  }
}

void DTLB::insert_to_dtlb(int index, int tag, int time, int pfn) {
  int oldest_used = INT32_MAX;
  int oldest_index = 0;
  for (int i = 0; i < set_size; i++) {
    if (dtlb[index][i].time_last_used < oldest_used) {
      oldest_index = i;
      oldest_used = dtlb[index][i].time_last_used;
    }
  }
  dtlb[index][oldest_index].time_last_used = time;
  dtlb[index][oldest_index].tag = tag;
  dtlb[index][oldest_index].pfn = pfn;
  dtlb[index][oldest_index].valid = true;
}

pair<bool, int> DTLB::check_dtlb(int index, int tag, int time) {
  for (int i = 0; i < set_size; i++) {
    if (dtlb[index][i].tag == tag && dtlb[index][i].valid == true) {
      dtlb[index][i].time_last_used = time;
      return {true, dtlb[index][i].pfn};
    }
  }
  return {false, -1};
}

void DTLB::remove_dtlb_entries_bc_pt_eviction(int vpn, int pfn) {
  for(int i = 0; i < set_count; i++){
    for(int j = 0; j < set_size; j++){
      if(dtlb[i][j].pfn == pfn && dtlb[i][j].valid){
       // cout << "| DTLB entry "<< i << " tag " << dtlb[i][j].pfn << " is now invalidated | ";
        dtlb[i][j].pfn = -1;
        dtlb[i][j].tag = -1;
        dtlb[i][j].time_last_used = -1;
        dtlb[i][j].valid = false;
      }
    }
  }
}
