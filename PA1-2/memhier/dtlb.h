#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#ifndef DTLB_H
#define DTLB_H
using namespace std;

class DTLB_entry {
public:
  int tag = -1;
  int pfn = -1;
  int time_last_used = -1;
  bool valid; 
  DTLB_entry(int t, int p);
};

class DTLB {
public:
  int set_count;
  int set_size;
  vector<vector<DTLB_entry>> dtlb;
  DTLB(int set_count, int set_size);
  void insert_to_dtlb(int index, int tag, int time, int ppn);
  pair<bool, int> check_dtlb(int index, int tag, int time);
  void remove_dtlb_entries_bc_pt_eviction(int vpn, int pfn);
};

#endif