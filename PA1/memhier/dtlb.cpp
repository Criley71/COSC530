#include "dtlb.h"
using namespace std;

DTLB_entry::DTLB_entry(int t, int p) {
  tag = t;
  ppn = p;
  timer = -1;
}

DTLB::DTLB(int sc, int ss) {
  set_count = sc;
  set_size = ss;
  dtlb.resize(set_count);
  for (int i = 0; i < set_count; i++) {
    dtlb[i].resize(set_count, DTLB_entry(-1, -1));
  }
}

void DTLB::insert_to_dtlb(int index, int tag, int time, int ppn) {
  int oldest_used = INT32_MAX;
  int oldest_index = 0;
  for (int i = 0; i < set_size; i++) {
    if (dtlb[index][i].timer < oldest_used) {
      oldest_index = i;
      oldest_used = dtlb[index][i].timer;
    }
  }
  dtlb[index][oldest_index].timer = time;
  dtlb[index][oldest_index].tag = tag;
  dtlb[index][oldest_index].ppn = ppn;
}

bool DTLB::check_dtlb(int index, int tag, int time) {
  for(int i = 0; i < set_size; i++){
    if(dtlb[index][i].tag == tag){
      dtlb[index][i].timer = time;
      return true;
    }
  }
  return false;
}