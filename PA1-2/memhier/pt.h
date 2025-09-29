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

class Page{
  public:
  bool valid = false;
  bool dirty = false;
  int pfn = -1;
  int time_last_used = -1;
  //vector<pair<int, int>> l2_index_tag_map; //if its in dc it has to be in l2 so we can just evict the l2 which will then chain react to dc
  //vector<pair<int, int>> dc_index_tag_map; //if its in dc it has to be in l2 so we can just evict the l2 which will then chain react to dc

  Page(int pt_l2_ratio);//need to size the l2 map to be the size of the ratio of l2 address that can fit in the page
};

class PT{
public:
  vector<Page> page_table;
  vector<int> vpns_in_use;
  int pfn_in_use_count = 0;
  PT(int virtual_page_count, int pt_l2_ratio, int physical_page_count);
  int check_page_table(int vpn, int timer, bool is_write);
  pair<bool, int> insert_page(int vpn, int vpc, int ppc, int timer, bool is_write, int page_offset, int page_offset_bits, bool l2_enabled, DC &cache, L2 &l2, int &disk_ref, int &mem_ref, double &l2_ref, int &page_refs,bool dtlb_enabled, DTLB &dtlb);
  int get_current_page_use_count();
  int translate_to_phys_address(int pfn, int page_offset, int page_offset_bits);

};

#endif