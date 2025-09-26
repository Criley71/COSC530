#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <bitset>
#include "dc.h"
#include "l2.h"
#include "dtlb.h"
#ifndef PT_H
#define PT_H
using namespace std;

class Page {
public:
  bool valid = false;
  int pfn = -1;
  bool dirty = false;
  int time = -1;
};

class PT {
public:
  int pfn_used_count;
  vector<Page> page_table;
  PT(int virtual_page_count);
  int check_page_table(int vpn, int timer, bool is_write); //return pfn or -1 if not found
  pair<int,bool> insert_page(int vpn, int vpc, int ppc, int timer, bool is_write, DC &cache, L2 &l2, int &disk_ref, DTLB &dtlb, int &mem_refs); //check if its full by seeing if pfn_used_count = phys pagec count, return pfn
  uint64_t vpn_to_phys_address(int vpn, int page_offset, int page_offset_bits, int vpc, int ppc, int timer, bool is_write, DC &cache, L2 &l2, int &disk_ref, DTLB &dtlb, int & mem_refs);
  int get_current_page_count();

};

#endif