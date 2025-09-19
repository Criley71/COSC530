#include <bitset>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>

#include "dc.h"
#include "dtlb.h"
#include "l2.h"
#include "pt.h"

using namespace std;
/*
dtlb = data translation lookaside buffer
dc = data cache
pt = page table
l2 = second level data cache
*/
class Config {
public:
  // data translation lookaside buffer
  int dtlb_set_count; // max 256
  int dtlb_set_size;
  int dltb_index_bits;
  bool dtlb_enabled;
  int dtlb_associativity;

  int page_size;
  int pt_offset_bit;
  int pt_index_bits;

  int virtual_page_count; // max 8192, must be power of 2
  int virtual_page_size;  // (bytes)
  int virtual_page_num;

  int physical_page_count; // max 1024, must be power of 2
  int physical_page_size;
  // int physical_page_bit_count;
  int physical_page_num;

  int dc_set_count; // max 8192
  int dc_set_size;
  int dc_associativity;
  int dc_line_size; // min 8 (bytes)
  bool dc_write_thru_no_allo;
  int dc_index_bits;
  int dc_offset_bits;

  int l2_set_count;
  int l2_set_size;
  int l2_associativity;
  int l2_line_size; // >= dc_line_size (bytes)
  bool l2_write_thru_no_allo;
  bool l2_enabled;
  int l2_index_bits;
  int l2_offset_bits;
  int counter = 0;
  bool virt_addr;
};

Config init(); // function to read in config and set variables
bool check_pwr_2(int val);
void print_config_after_read(Config config);
void read_data_file(Config config);
void print_stats(double dtlb_hits, double dtlb_misses, double pt_hits, double pt_faults, double dc_hits, double dc_misses, double l2_hits, double l2_misses, double total_reads, double total_writes, double mem_refs, double pt_refs, double disk_refs);
pair<int, int> tlb_index_tag_getter(int vpn, int tlb_set_count);
int main(int argc, char *argv[]) {
  Config config = init();
  print_config_after_read(config);
  read_data_file(config);
}

Config init() {
  Config config;
  ifstream fin("trace.config");
  config.counter = 0;
  string line;
  string val;
  int colon_index;

  while (getline(fin, line)) {
    if (!line.empty()) {
      if (line.find("Data TLB configuration") == 0) { // looks for dtlb header
        for (int i = 0; i < 2; i++) {
          getline(fin, line); // gets next line in dltb head, should be "Numer of sets:"
          if (line.find("Number of sets") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            config.dtlb_set_count = stoi(val);

            val = "";
            if (!check_pwr_2(config.dtlb_set_count)) {
              cout << "DTLB set count is not a power of 2. Exiting\n";
              exit(-1);
            }
            config.dltb_index_bits = log2(config.dtlb_set_count);
          } else if (line.find("Set size") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            config.dtlb_set_size = stoi(val);
            val = "";
          }
        }
      } else if (line.find("Page Table configuration") == 0) {
        for (int i = 0; i < 3; i++) {
          getline(fin, line);
          if (line.find("Number of virtual pages") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            config.virtual_page_count = stoi(val);

            val = "";
            if (!check_pwr_2(config.virtual_page_count)) {
              cout << "Number of virtual pages is not a power 2. Exiting\n";
              exit(-1);
            }
            config.pt_index_bits = log2(config.virtual_page_count);

          } else if (line.find("Number of physical pages") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            config.physical_page_count = stoi(val);

            val = "";
            if (!check_pwr_2(config.physical_page_count)) {
              cout << "Number of Physical pages is not a power 2. Exiting\n";
              exit(-1);
            }
          } else if (line.find("Page size") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            config.page_size = stoi(val);

            val = "";
            if (!check_pwr_2(config.page_size)) {
              cout << "Page size is not a power 2. Exiting\n";
              exit(-1);
            }
            config.pt_offset_bit = log2(config.page_size);
          }
        }
      } else if (line.find("Data Cache configuration") == 0) {
        for (int i = 0; i < 4; i++) {
          getline(fin, line);
          if (line.find("Number of sets") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            config.dc_set_count = stoi(val);
            val = "";
            if (!check_pwr_2(config.dc_set_count)) {
              cout << "Number of DC sets is not a power of 2. Exiting\n";
              exit(-1);
            }
            config.dc_index_bits = log2(config.dc_set_count);
          } else if (line.find("Set size") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            config.dc_set_size = stoi(val);
            val = "";
            /*
            if (!check_pwr_2(config.dc_set_size)) {
              cout << "Number of DC entries is not a power of 2. Exiting\n";
              exit(-1);
            }
              */
          } else if (line.find("Line size") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            config.dc_line_size = stoi(val);
            val = "";
            if (!check_pwr_2(config.dc_line_size)) {
              cout << "Number of DC line size is not a power of 2. Exiting\n";
              exit(-1);
            }
            config.dc_offset_bits = log2(config.dc_line_size);
          } else if (line.find("Write through/no write allocate") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            if (val == "n") {
              config.dc_write_thru_no_allo = false;
            } else {
              config.dc_write_thru_no_allo = true;
            }
            val = "";
          }
        }
      } else if (line.find("L2 Cache configuration") == 0) {
        for (int i = 0; i < 4; i++) {
          getline(fin, line);
          if (line.find("Number of sets") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            config.l2_set_count = stoi(val);
            val = "";
            if (!check_pwr_2(config.l2_set_count)) {
              cout << "Number of sets in the L2 cache are not a power of 2. Exiting\n";
              exit(-1);
            }
            config.l2_index_bits = log2(config.l2_set_count);
          } else if (line.find("Set size") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            config.l2_set_size = stoi(val);
            val = "";
          } else if (line.find("Line size") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            config.l2_line_size = stoi(val);
            val = "";
            if (!check_pwr_2(config.l2_line_size)) {
              cout << "L2 line size is not a power of 2. Exiting\n";
              exit(-1);
            }
            if (config.l2_line_size < config.dc_line_size) {
              cout << "L2 line size is smaller dc line size. Exiting\n";
              exit(-1);
            }
            config.l2_offset_bits = log2(config.l2_line_size);
          } else if (line.find("Write through/no write allocate") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            if (val == "n") {
              config.l2_write_thru_no_allo = false;
            } else if (val == "y") {
              config.l2_write_thru_no_allo = true;
            }
            val = "";
          }
        }
      } else if (line.find("Virtual addresses") == 0) {
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        if (val == "y") {
          config.virt_addr = true;
        } else if (val == "n") {
          config.virt_addr = false;
        }
        val = "";
      } else if (line.find("TLB") == 0) {
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        if (val == "y") {
          config.dtlb_enabled = true;
        } else if (val == "n") {
          config.dtlb_enabled = false;
        }
        val = "";
      } else if (line.find("L2 cache") == 0) {
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        if (val == "y") {
          config.l2_enabled = true;
        } else if (val == "n") {
          config.l2_enabled = false;
        }
        val = "";
      }
    }
  }
  return config;
}

bool check_pwr_2(int val) { // 8 = 1000, 7 = 0111 ---> 1000 & 0111 = 0000
  return (0 == ((val - 1) & val));
}

void print_config_after_read(Config config) {
  cout << "Data TLB contains " << config.dtlb_set_count << " sets.\n";
  cout << "Each set contains " << config.dtlb_set_size << " entries.\n";
  cout << "Number of bits used for the index is " << config.dltb_index_bits << ".\n\n";
  cout << "Number of virtual pages is " << config.virtual_page_count << ".\n";
  cout << "Number of physical pages is " << config.physical_page_count << ".\n";
  cout << "Each page contains " << config.page_size << " bytes.\n";
  cout << "Number of bits used for the page table index is " << config.pt_index_bits << ".\n";
  cout << "Number of bits used for the page offset is " << config.pt_offset_bit << ".\n\n";
  cout << "D-cache contains " << config.dc_set_count << " sets.\n";
  cout << "Each set contains " << config.dc_set_size << " entries.\n";
  cout << "Each line is " << config.dc_line_size << " bytes.\n";
  if (config.dc_write_thru_no_allo) {
    cout << "The cache uses a no write-allocate and write-through policy.\n";
  } else {
    cout << "The cache uses a write-allocate and write-back policy.\n";
  }
  cout << "Number of bits used for the index is " << config.dc_index_bits << ".\n";
  cout << "Number of bits used for the offset is " << config.dc_offset_bits << ".\n\n";
  cout << "L2-cache contains " << config.l2_set_count << " sets.\n";
  cout << "Each set contains " << config.l2_set_size << " entries.\n";
  cout << "Each line is " << config.l2_line_size << " bytes.\n";
  if (config.l2_write_thru_no_allo) {
    cout << "The cache uses a no-write-allocate and write through policy.\n";
  } else {
    cout << "The cache uses a write-allocate and write-back policy.\n";
  }
  cout << "Number of bits used for the index is " << config.l2_index_bits << ".\n";
  cout << "Number of bits used for the offset is " << config.l2_offset_bits << ".\n\n";
  if (config.virt_addr) {
    cout << "The addresses read in are virtual addresses.\n";
  } else {
    cout << "The addresses read in are physical addresses.\n";
  }
  if (!config.dtlb_enabled) {
    cout << "TLB is disabled in this configuration.\n";
  }
  if (!config.l2_enabled) {
    cout << "L2 cache is disabled in this configuration.";
  }

  cout << "\n";
  if (config.virt_addr) {
    cout << "Virtual  Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n";
  } else {
    cout << "Physical Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n";
  }
  cout << "Address  Page # Off  Tag    Ind Res. Res. Pg # DC Tag Ind Res. L2 Tag Ind Res.\n";
  cout << "-------- ------ ---- ------ --- ---- ---- ---- ------ --- ---- ------ --- ----\n";
}

void read_data_file(Config config) {
  double dtlb_hits = 0;
  double dtlb_misses = 0;
  double pt_hits = 0;
  double pt_faults = 0;
  double dc_hits = 0;
  double dc_misses = 0;
  double l2_hits = 0;
  double l2_misses = 0;
  double total_reads = 0;
  double total_writes = 0;
  int memory_refs = 0;
  int pt_refs = 0;
  int disk_refs = 0;
  int address;
  int virt_page_num;
  string virt_page_num_bin;
  int page_off;
  string page_off_bin;
  int tlb_tag;
  int tlb_index;
  string tlb_res;
  string pt_res;
  int phys_page_num;
  string phys_page_num_bin;
  int dc_tag;
  string dc_tag_bin;
  int dc_index;
  string dc_index_bin;
  int l2_tag;
  string l2_tag_bin;
  int l2_index;
  string l2_index_bin;
  int dc_res;
  bool is_read; // false will be on write, true on read
  string l2_res;
  int virtual_page_calc;
  string virtual_page_calc_bin;
  string virtual_page_calc_hex;
  int pfn;
  bool page_was_dirty;
  ifstream fin("long_trace.dat");
  string line;
  string hex_val;
  string bin_string;
  string p_bin_string;
  string physical_address_bin;
  pair<int, int> tlb_index_and_tag;
  bool was_l2_full = false;
  stringstream ss;
  bitset<64> b;
  bitset<64> p;
  DC DATA_CACHE = DC(config.dc_set_count, config.dc_set_size, config.dc_line_size, config.dc_write_thru_no_allo, config.dc_index_bits, config.dc_offset_bits);
  L2 L2_CACHE = L2(config.l2_set_count, config.l2_set_size, config.l2_line_size, config.l2_write_thru_no_allo, config.l2_index_bits, config.l2_offset_bits);
  PT PAGE_TABLE = PT(config.virtual_page_count);
  DTLB translation_buffer = DTLB(config.dtlb_set_count, config.dtlb_set_size);
  int dirty_bit;
  while (getline(fin, line)) {
    config.counter += 1; // i dont remember why i made this part of the config class but its too late to change
    if (line[0] == 'R') {
      is_read = true;
      dirty_bit = 0;
      total_reads += 1;
    } else if (line[0] == 'W') {
      is_read = false;
      dirty_bit = 1;
      total_writes += 1;
    }
    hex_val = line.substr(2, line.size());
    ss << hex << hex_val;
    unsigned temp; // temp var that will hold all string stream values
    // I learned how stringstreams work doing this, this is awesome
    // No more hex translation helper functions!

    ss >> temp;
    ss.clear();
    b = bitset<64>(temp);
    bin_string = b.to_string();

    was_l2_full = false;
    if (!config.dc_write_thru_no_allo && !config.l2_write_thru_no_allo) {
      if (!config.virt_addr) {
        // CALCULATE PHYSICAL PAGE NUM TO MAKE SURE ITS IN BOUNDS
        phys_page_num_bin = bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);

        if (phys_page_num >= config.physical_page_count) {
          cout << "hierarchy: physical address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        cout << hex << setw(8) << setfill('0') << temp;
        ss.clear();
        cout << " " << setw(7) << setfill(' ');

        // PAGE OFFSET CALCULATION AND PRINTING
        page_off_bin = bin_string.substr(64 - (config.pt_offset_bit));
        page_off = stoi(page_off_bin, 0, 2);
        cout << " " << setw(4) << hex << page_off;
        // No TLB so print 21 spaces
        cout << " " << setw(21);

        // PHYS PAGE NUM PRINT
        cout << " " << setw(4) << setfill(' ') << hex << phys_page_num;

        // DATA CACHE TAG CALC & PRINT
        dc_tag_bin = bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DATA CACHE INDEX CALC & PRINT
        dc_index_bin = bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        dc_index = stoi(dc_index_bin, 0, 2);
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;

        // check data cache for block, if hit no need to check l2
        // to be a hit, it would have to originally missed meaning it would then be in l2
        // just need to have it that if l2 block is evicted the corresponding dc block is too
        if (DATA_CACHE.check_cache(dc_index, dc_tag, config.counter, !is_read, -1, false)) {
          l2_tag_bin = bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
          l2_tag = stoi(l2_tag_bin, 0, 2);
          l2_index_bin = bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
          l2_index = stoi(l2_index_bin, 0, 2);
          // L2_CACHE.update_access_time(l2_index, l2_tag, config.counter);

          dc_hits += 1;
          cout << " hit  \n";
          continue;
        } else {
          pair<bool, string> was_dc_replaced_and_was_dirty = DATA_CACHE.insert_to_cache(dc_index, dc_tag, config.counter, dirty_bit, -1, bin_string);
          bool dirty_replaced = was_dc_replaced_and_was_dirty.first;
          string check_for_replace = was_dc_replaced_and_was_dirty.second;

          dc_misses += 1;
          if (config.l2_enabled) {

            if (dirty_replaced == true) {
              l2_hits += 1;
              string old_address = was_dc_replaced_and_was_dirty.second;
              l2_tag_bin = old_address.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
              l2_tag = stoi(l2_tag_bin, 0, 2);
              l2_index_bin = old_address.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
              l2_index = stoi(l2_index_bin, 0, 2);
              L2_CACHE.update_dirty_bit(l2_index, l2_tag, config.counter);
              // ME NOT ORIGINALLY HAVING THIS COUNTER ACTUALLY BROKE
              // THE WHOLE THING AS SOMEHOW 2 L2'S WOULD HAVE THE SAME
              // ACCESS TIME COUNT
              config.counter += 1;

              // cout << "TEST";
              // l2_hits += 1;
              //
            }
            cout << " miss";
          } else {
            cout << " miss ";
          }
        }
        // if l2 is disabled then the line is done
        if (!config.l2_enabled) {
          cout << "\n";
        } else {
          // L2 TAG CALC & PRINT
          l2_tag_bin = bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
          l2_tag = stoi(l2_tag_bin, 0, 2);
          cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";

          // L2 INDEX CALC & PRINT
          l2_index_bin = bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
          l2_index = stoi(l2_index_bin, 0, 2);
          cout << setw(3) << setfill(' ') << hex << l2_index;
          was_l2_full = L2_CACHE.check_if_index_is_full(l2_index);
          if (is_read) {
            dirty_bit = 0;
          } else {
            dirty_bit = 1;
          }
          if (l2_index == 0x8) {

            // cout << " | current counter " << dec << config.counter << " |  ";
          }
          if (L2_CACHE.check_l2(l2_index, l2_tag, config.counter, dirty_bit, -1, false, dc_index, dc_tag)) {
            cout << " hit \n";
            l2_hits += 1;
          } else {
            l2_misses += 1;
            int temp_counter_to_see_if_l2_incremented = memory_refs;
            pair<bool, string> was_l2_replaced_and_if_yes_dc_phys_address = L2_CACHE.insert_to_l2(l2_index, l2_tag, config.counter, dirty_bit, -1, dc_index, dc_tag, bin_string, memory_refs);
            bool was_there_an_l2_eviction = was_l2_replaced_and_if_yes_dc_phys_address.first;
            if (was_there_an_l2_eviction) {

              string replace_address = was_l2_replaced_and_if_yes_dc_phys_address.second;
              dc_tag_bin = replace_address.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
              dc_tag = stoi(dc_tag_bin, 0, 2);
              dc_index_bin = replace_address.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
              dc_index = stoi(dc_index_bin, 0, 2);
              bool was_dc_dirty = DATA_CACHE.evict_given_l2_phys_address(dc_index, dc_tag);
              if (was_dc_dirty == true) {
                l2_hits += 1;
              }
              if (was_dc_dirty && memory_refs == temp_counter_to_see_if_l2_incremented) {
                memory_refs += 1;
              }
            }
            memory_refs += 1;
            cout << " miss\n";
          }
        }
      } else if (config.virt_addr && !config.dtlb_enabled) { // maybe just change this section for va enabled but dtlb disabled
        // make if else for dtlb being enabled. or just reorder this
        //  VIRTUAL ADDRESS AND PAGE NUMBER PRINTING
        virt_page_num_bin = bin_string.substr(0, 64 - config.pt_offset_bit);
        virt_page_num = stoi(virt_page_num_bin, 0, 2);
        if (virt_page_num >= config.virtual_page_count) {
          cout << "hierarchy: virtual address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        cout << hex << setw(8) << setfill('0') << temp;
        cout << setfill(' ') << setw(7) << hex << virt_page_num << " ";

        // PAGE OFFSET CALCULATION AND PRINTING
        page_off_bin = bin_string.substr(64 - config.pt_offset_bit);
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << setfill(' ') << page_off;

        // PAGE TABLE RESULT PRINTING
        pfn = PAGE_TABLE.check_page_table(virt_page_num);
        // cout << " " << PAGE_TABLE.page_table[virt_page_num].valid << "<-isvalid";
        if (pfn == -1) {
          pfn = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE).first;
          // cout << "wasnt valid so insert with pfn value->" << pfn << " pfn used count=" << PAGE_TABLE.pfn_used_count << " ";
          // page_was_dirty = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read).second;
          pt_faults += 1;
          pt_res = "miss";
          pt_refs += 1;
          disk_refs += 1;
        } else {
          pt_refs += 1;
          pt_hits += 1;
          pt_res = "hit ";
        }
        if (config.dtlb_enabled) {
          tlb_index_and_tag = tlb_index_tag_getter(virt_page_num, config.dtlb_set_count);
          // cout << "tlb_index = " << tlb_index_and_tag.first << " tlb_tag = " << tlb_index_and_tag.second << " ";
          tlb_index = tlb_index_and_tag.first;
          tlb_tag = tlb_index_and_tag.second;
          cout << setw(7) << setfill(' ') << hex << tlb_tag;
          cout << setw(4) << setfill(' ') << hex << tlb_index;
          if (translation_buffer.check_dtlb(tlb_index, tlb_tag, config.counter)) {
            cout << setw(4) << setfill(' ') << " hit ";
            cout << setw(5) << setfill(' ') << " ";
          } else {
            cout << setw(4) << setfill(' ') << " miss";
            translation_buffer.insert_to_dtlb(tlb_index, tlb_tag, config.counter, pfn);
            cout << setw(5) << setfill(' ') << pt_res;
          }
        } else {

          cout << setw(21) << setfill(' ') << pt_res;
        }

        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = PAGE_TABLE.vpn_to_phys_address(virt_page_num, page_off, config.pt_offset_bit, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        phys_page_num_bin = p_bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = p_bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = p_bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        dc_index = stoi(dc_index_bin, 0, 2);
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;

        // check data cache for block, if hit no need to insert
        if (DATA_CACHE.check_cache(dc_index, dc_tag, config.counter, !is_read, pfn, false)) {
          cout << " hit  \n";
          dc_hits += 1;
          continue;
        } else {
          pair<bool, string> was_dc_replaced_and_was_dirty = DATA_CACHE.insert_to_cache(dc_index, dc_tag, config.counter, dirty_bit, pfn, p_bin_string);
          bool dirty_replaced = was_dc_replaced_and_was_dirty.first;
          string check_for_replace = was_dc_replaced_and_was_dirty.second;
          dc_misses += 1;
          if (config.l2_enabled) {
            if (dirty_replaced == true) {
              l2_hits += 1;
              string old_address = was_dc_replaced_and_was_dirty.second;
              l2_tag_bin = old_address.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
              l2_tag = stoi(l2_tag_bin, 0, 2);
              l2_index_bin = old_address.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
              l2_index = stoi(l2_index_bin, 0, 2);
              L2_CACHE.update_dirty_bit(l2_index, l2_tag, config.counter);
              // ME NOT ORIGINALLY HAVING THIS COUNTER ACTUALLY BROKE
              // THE WHOLE THING AS SOMEHOW 2 L2'S WOULD HAVE THE SAME
              // ACCESS TIME COUNT
              config.counter += 1;
            }
          }
          cout << " miss ";
        }
        if (!config.l2_enabled) {
          cout << "\n";
        } else {
          l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
          l2_tag = stoi(l2_tag_bin, 0, 2);
          cout << " " << setw(5) << setfill(' ') << hex << l2_tag << " ";

          // L2 INDEX CALC & PRINT
          l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
          l2_index = stoi(l2_index_bin, 0, 2);
          cout << setw(3) << setfill(' ') << hex << l2_index;
          int dirty_bit;
          if (is_read) {
            dirty_bit = 0;
          } else {
            dirty_bit = 1;
          }
          if (L2_CACHE.check_l2(l2_index, l2_tag, config.counter, dirty_bit, pfn, false, dc_index, dc_tag)) {
            cout << " hit \n";
            l2_hits += 1;
          } else {
            l2_misses += 1;
            int temp_counter_to_see_if_l2_incremented = memory_refs;
            pair<bool, string> was_l2_replaced_if_yes_dc_phys_address = L2_CACHE.insert_to_l2(l2_index, l2_tag, config.counter, dirty_bit, pfn, dc_index, dc_tag, p_bin_string, memory_refs);
            bool was_there_an_l2_eviction = was_l2_replaced_if_yes_dc_phys_address.first;
            if (was_there_an_l2_eviction) {
              string replace_address = was_l2_replaced_if_yes_dc_phys_address.second;
              dc_tag_bin = replace_address.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
              dc_tag = stoi(dc_tag_bin, 0, 2);
              dc_index_bin = replace_address.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
              dc_index = stoi(dc_index_bin, 0, 2);
              bool was_dc_dirty = DATA_CACHE.evict_given_l2_phys_address(dc_index, dc_tag);
              if (was_dc_dirty == true) {
                l2_hits += 1;
              }
              if (was_dc_dirty && memory_refs == temp_counter_to_see_if_l2_incremented) {
                memory_refs += 1;
                // need 1 for writing dirty l2 to main memory, but also only once if the l2 evicted was dirty its incremented in the function
              }
            }
            memory_refs += 1;
            cout << " miss\n";
          }
          /*
            if (L2_CACHE.check_l2(l2_index, l2_tag, config.counter, 1, pfn, false, dc_index, dc_tag)) {
              cout << " hit \n";
            } else {
              cout << " miss\n";
            }
          */
        }
      } else if (config.virt_addr && config.dtlb_enabled) {
        // VIRTUAL ADDRESS AND PAGE NUMBER PRINTING
        virt_page_num_bin = bin_string.substr(0, 64 - config.pt_offset_bit);
        virt_page_num = stoi(virt_page_num_bin, 0, 2);
        if (virt_page_num >= config.virtual_page_count) {
          cout << "hierarchy: virtual address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        cout << hex << setw(8) << setfill('0') << temp;
        cout << setfill(' ') << setw(7) << hex << virt_page_num << " ";

        // PAGE OFFSET CALCULATION AND PRINTING
        page_off_bin = bin_string.substr(64 - config.pt_offset_bit);
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << setfill(' ') << page_off;

        tlb_index_and_tag = tlb_index_tag_getter(virt_page_num, config.dtlb_set_count);
        // cout << "tlb_index = " << tlb_index_and_tag.first << " tlb_tag = " << tlb_index_and_tag.second << " ";
        tlb_index = tlb_index_and_tag.first;
        tlb_tag = tlb_index_and_tag.second;
        cout << setw(7) << setfill(' ') << hex << tlb_tag;
        cout << setw(4) << setfill(' ') << hex << tlb_index;
        if (translation_buffer.check_dtlb(tlb_index, tlb_tag, config.counter)) {
          cout << setw(4) << setfill(' ') << " hit ";
          cout << setw(5) << setfill(' ') << " ";
          dtlb_hits += 1;
        } else {
          cout << setw(4) << setfill(' ') << " miss";
          dtlb_misses += 1;
          pfn = PAGE_TABLE.check_page_table(virt_page_num);
          // cout << " " << PAGE_TABLE.page_table[virt_page_num].valid << "<-isvalid";
          if (pfn == -1) {
            pfn = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE).first;
            // cout << "wasnt valid so insert with pfn value->" << pfn << " pfn used count=" << PAGE_TABLE.pfn_used_count << " ";
            // page_was_dirty = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read).second;
            pt_faults += 1;
            pt_res = "miss";
            pt_refs += 1;
            disk_refs += 1;
          } else {
            pt_refs += 1;
            pt_hits += 1;
            pt_res = "hit ";
          }
          translation_buffer.insert_to_dtlb(tlb_index, tlb_tag, config.counter, pfn);
          cout << setw(5) << setfill(' ') << pt_res;
        }
        /*
        // PAGE TABLE RESULT PRINTING
        pfn = PAGE_TABLE.check_page_table(virt_page_num);
        // cout << " " << PAGE_TABLE.page_table[virt_page_num].valid << "<-isvalid";
        if (pfn == -1) {
          pfn = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE).first;
          // cout << "wasnt valid so insert with pfn value->" << pfn << " pfn used count=" << PAGE_TABLE.pfn_used_count << " ";
          // page_was_dirty = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read).second;
          pt_faults += 1;
          pt_res = "miss";
          pt_refs += 1;
          disk_refs += 1;
        } else {
          pt_refs += 1;
          pt_hits += 1;
          pt_res = "hit ";
        }
        */
        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = PAGE_TABLE.vpn_to_phys_address(virt_page_num, page_off, config.pt_offset_bit, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        phys_page_num_bin = p_bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = p_bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = p_bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        dc_index = stoi(dc_index_bin, 0, 2);
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;

        // check data cache for block, if hit no need to insert
        if (DATA_CACHE.check_cache(dc_index, dc_tag, config.counter, !is_read, pfn, false)) {
          cout << " hit  \n";
          dc_hits += 1;
          continue;
        } else {
          pair<bool, string> was_dc_replaced_and_was_dirty = DATA_CACHE.insert_to_cache(dc_index, dc_tag, config.counter, dirty_bit, pfn, p_bin_string);
          bool dirty_replaced = was_dc_replaced_and_was_dirty.first;
          string check_for_replace = was_dc_replaced_and_was_dirty.second;
          dc_misses += 1;
          if (config.l2_enabled) {
            if (dirty_replaced == true) {
              l2_hits += 1;
              string old_address = was_dc_replaced_and_was_dirty.second;
              l2_tag_bin = old_address.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
              l2_tag = stoi(l2_tag_bin, 0, 2);
              l2_index_bin = old_address.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
              l2_index = stoi(l2_index_bin, 0, 2);
              L2_CACHE.update_dirty_bit(l2_index, l2_tag, config.counter);
              // ME NOT ORIGINALLY HAVING THIS COUNTER ACTUALLY BROKE
              // THE WHOLE THING AS SOMEHOW 2 L2'S WOULD HAVE THE SAME
              // ACCESS TIME COUNT
              config.counter += 1;
            }
          }
          cout << " miss ";
        }
        if (!config.l2_enabled) {
          cout << "\n";
        } else {
          l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
          l2_tag = stoi(l2_tag_bin, 0, 2);
          cout << " " << setw(5) << setfill(' ') << hex << l2_tag << " ";

          // L2 INDEX CALC & PRINT
          l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
          l2_index = stoi(l2_index_bin, 0, 2);
          cout << setw(3) << setfill(' ') << hex << l2_index;
          int dirty_bit;
          if (is_read) {
            dirty_bit = 0;
          } else {
            dirty_bit = 1;
          }
          if (L2_CACHE.check_l2(l2_index, l2_tag, config.counter, dirty_bit, pfn, false, dc_index, dc_tag)) {
            cout << " hit \n";
            l2_hits += 1;
          } else {
            l2_misses += 1;
            int temp_counter_to_see_if_l2_incremented = memory_refs;
            pair<bool, string> was_l2_replaced_if_yes_dc_phys_address = L2_CACHE.insert_to_l2(l2_index, l2_tag, config.counter, dirty_bit, pfn, dc_index, dc_tag, p_bin_string, memory_refs);
            bool was_there_an_l2_eviction = was_l2_replaced_if_yes_dc_phys_address.first;
            if (was_there_an_l2_eviction) {
              string replace_address = was_l2_replaced_if_yes_dc_phys_address.second;
              dc_tag_bin = replace_address.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
              dc_tag = stoi(dc_tag_bin, 0, 2);
              dc_index_bin = replace_address.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
              dc_index = stoi(dc_index_bin, 0, 2);
              bool was_dc_dirty = DATA_CACHE.evict_given_l2_phys_address(dc_index, dc_tag);
              if (was_dc_dirty == true) {
                l2_hits += 1;
              }
              if (was_dc_dirty && memory_refs == temp_counter_to_see_if_l2_incremented) {
                memory_refs += 1;
                // need 1 for writing dirty l2 to main memory, but also only once if the l2 evicted was dirty its incremented in the function
              }
            }
            memory_refs += 1;
            cout << " miss\n";
          }
          /*
            if (L2_CACHE.check_l2(l2_index, l2_tag, config.counter, 1, pfn, false, dc_index, dc_tag)) {
              cout << " hit \n";
            } else {
              cout << " miss\n";
            }
          */
        }
      }
    } else if (config.dc_write_thru_no_allo && !config.l2_write_thru_no_allo) {
      // data cache is write through no write allocate but l2 isnt
      if (!config.virt_addr) {
        phys_page_num_bin = bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);

        if (phys_page_num >= config.physical_page_count) {
          cout << "hierarchy: physical address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        cout << hex << setw(8) << setfill('0') << temp;
        ss.clear();
        cout << " " << setw(7) << setfill(' ');

        // PAGE OFFSET CALCULATION AND PRINTING
        page_off_bin = bin_string.substr(64 - (config.pt_offset_bit));
        page_off = stoi(page_off_bin, 0, 2);
        cout << " " << setw(4) << hex << page_off;
        // No TLB so print 21 spaces
        cout << " " << setw(21);

        // PHYS PAGE NUM PRINT
        cout << " " << setw(4) << setfill(' ') << hex << phys_page_num;

        // DATA CACHE TAG CALC & PRINT
        dc_tag_bin = bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DATA CACHE INDEX CALC & PRINT
        dc_index_bin = bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        dc_index = stoi(dc_index_bin, 0, 2);
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;
        if (DATA_CACHE.check_cache(dc_index, dc_tag, config.counter, !is_read, -1, false)) {
          // datacache was found but if it was a write we need to also update the l2 dirty bit & access time
          // hit
          if (!is_read) { // hit write
            l2_tag_bin = bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            l2_index = stoi(l2_index_bin, 0, 2);
            L2_CACHE.update_dirty_bit(l2_index, l2_tag, config.counter);
            // l2_hits += 1; weird imo, wouldnt this be accessessing and hitting the l2 ?
            config.counter += 1;
          }
          dc_hits += 1;
          cout << " hit ";
          if (is_read) {
            cout << " \n";
            continue;
          }
        } else {
          if (is_read) {
            DATA_CACHE.insert_to_cache(dc_index, dc_tag, config.counter, dirty_bit, -1, phys_page_num_bin);
          }
          dc_misses += 1;
          if (config.l2_enabled) {
            cout << " miss";
          } else {
            cout << " miss ";
          }
        }
        if (!config.l2_enabled) {
          cout << "\n";
        } else {
          l2_tag_bin = bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
          l2_tag = stoi(l2_tag_bin, 0, 2);
          cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";

          // L2 INDEX CALC & PRINT
          l2_index_bin = bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
          l2_index = stoi(l2_index_bin, 0, 2);
          cout << setw(3) << setfill(' ') << hex << l2_index;
          was_l2_full = L2_CACHE.check_if_index_is_full(l2_index);
          if (L2_CACHE.check_l2(l2_index, l2_tag, config.counter, dirty_bit, -1, false, dc_index, dc_tag)) {
            cout << " hit \n";
            l2_hits += 1;
          } else {
            l2_misses += 1;
            pair<bool, string> was_l2_replaced_and_if_yes_dc_phys_address = L2_CACHE.insert_to_l2(l2_index, l2_tag, config.counter, dirty_bit, -1, dc_index, dc_tag, bin_string, memory_refs);
            // the insert to l2 function will fully determine if mem ref incrememnt as it checks in the function for the dirty bit
            bool was_there_an_l2_eviction = was_l2_replaced_and_if_yes_dc_phys_address.first;
            if (was_there_an_l2_eviction) {
              string replace_address = was_l2_replaced_and_if_yes_dc_phys_address.second;
              dc_tag_bin = replace_address.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
              dc_tag = stoi(dc_tag_bin, 0, 2);
              dc_index_bin = replace_address.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
              dc_index = stoi(dc_index_bin, 0, 2);
              DATA_CACHE.evict_given_l2_phys_address(dc_index, dc_tag);
              // dc never is dirty as any write writes to l2
            }
            memory_refs += 1;
            cout << " miss\n";
          }
        }
      } else if (config.virt_addr && !config.dtlb_enabled) {
        virt_page_num_bin = bin_string.substr(0, 64 - config.pt_offset_bit);
        virt_page_num = stoi(virt_page_num_bin, 0, 2);
        if (virt_page_num >= config.virtual_page_count) {
          cout << "hierarchy: virtual address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        cout << hex << setw(8) << setfill('0') << temp;
        cout << setfill(' ') << setw(7) << hex << virt_page_num << " ";

        // PAGE OFFSET CALCULATION AND PRINTING
        page_off_bin = bin_string.substr(64 - config.pt_offset_bit);
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << setfill(' ') << page_off;

        // PAGE TABLE RESULT PRINTING
        pfn = PAGE_TABLE.check_page_table(virt_page_num);
        // cout << " " << PAGE_TABLE.page_table[virt_page_num].valid << "<-isvalid";
        if (pfn == -1) {
          pfn = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE).first;
          // cout << "wasnt valid so insert with pfn value->" << pfn << " pfn used count=" << PAGE_TABLE.pfn_used_count << " ";
          // page_was_dirty = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read).second;
          pt_faults += 1;
          pt_res = "miss";
          pt_refs += 1;
          disk_refs += 1;
        } else {
          pt_refs += 1;
          pt_hits += 1;
          pt_res = "hit ";
        }
        if (config.dtlb_enabled) {
          tlb_index_and_tag = tlb_index_tag_getter(virt_page_num, config.dtlb_set_count);
          // cout << "tlb_index = " << tlb_index_and_tag.first << " tlb_tag = " << tlb_index_and_tag.second << " ";
          tlb_index = tlb_index_and_tag.first;
          tlb_tag = tlb_index_and_tag.second;
          cout << setw(7) << setfill(' ') << hex << tlb_tag;
          cout << setw(4) << setfill(' ') << hex << tlb_index;
          if (translation_buffer.check_dtlb(tlb_index, tlb_tag, config.counter)) {
            cout << setw(4) << setfill(' ') << " hit ";
            cout << setw(5) << setfill(' ') << " ";
          } else {
            cout << setw(4) << setfill(' ') << " miss";
            translation_buffer.insert_to_dtlb(tlb_index, tlb_tag, config.counter, pfn);
            cout << setw(5) << setfill(' ') << pt_res;
          }
        } else {
          cout << setw(21) << setfill(' ') << pt_res;
        }

        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = PAGE_TABLE.vpn_to_phys_address(virt_page_num, page_off, config.pt_offset_bit, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        phys_page_num_bin = p_bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = p_bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = p_bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        dc_index = stoi(dc_index_bin, 0, 2);
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;
        if (DATA_CACHE.check_cache(dc_index, dc_tag, config.counter, !is_read, -1, false)) {
          // datacache was found but if it was a write we need to also update the l2 dirty bit & access time
          // hit
          if (!is_read) { // hit write
            l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            l2_index = stoi(l2_index_bin, 0, 2);
            L2_CACHE.update_dirty_bit(l2_index, l2_tag, config.counter);
            // l2_hits += 1; weird imo, wouldnt this be accessessing and hitting the l2 ?
            config.counter += 1;
          }
          dc_hits += 1;
          cout << " hit ";
          if (is_read) {
            cout << " \n";
            continue;
          }
        } else {
          if (is_read) {
            DATA_CACHE.insert_to_cache(dc_index, dc_tag, config.counter, dirty_bit, pfn, phys_page_num_bin);
          }
          dc_misses += 1;
          if (config.l2_enabled) {
            cout << " miss";
          } else {
            cout << " miss ";
          }
        }
        if (!config.l2_enabled) {
          cout << "\n";
        } else {
          l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
          l2_tag = stoi(l2_tag_bin, 0, 2);
          cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";

          // L2 INDEX CALC & PRINT
          l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
          l2_index = stoi(l2_index_bin, 0, 2);
          cout << setw(3) << setfill(' ') << hex << l2_index;
          was_l2_full = L2_CACHE.check_if_index_is_full(l2_index);
          if (L2_CACHE.check_l2(l2_index, l2_tag, config.counter, dirty_bit, pfn, false, dc_index, dc_tag)) {
            cout << " hit \n";
            l2_hits += 1;
          } else {
            l2_misses += 1;
            pair<bool, string> was_l2_replaced_and_if_yes_dc_phys_address = L2_CACHE.insert_to_l2(l2_index, l2_tag, config.counter, dirty_bit, pfn, dc_index, dc_tag, p_bin_string, memory_refs);
            // the insert to l2 function will fully determine if mem ref incrememnt as it checks in the function for the dirty bit
            bool was_there_an_l2_eviction = was_l2_replaced_and_if_yes_dc_phys_address.first;
            if (was_there_an_l2_eviction) {
              string replace_address = was_l2_replaced_and_if_yes_dc_phys_address.second;
              dc_tag_bin = replace_address.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
              dc_tag = stoi(dc_tag_bin, 0, 2);
              dc_index_bin = replace_address.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
              dc_index = stoi(dc_index_bin, 0, 2);
              DATA_CACHE.evict_given_l2_phys_address(dc_index, dc_tag);
              // dc never is dirty as any write writes to l2
            }
            memory_refs += 1;
            cout << " miss\n";
          }
        }
      } else if (config.virt_addr && config.dtlb_enabled) {
        // VIRTUAL ADDRESS AND PAGE NUMBER PRINTING
        virt_page_num_bin = bin_string.substr(0, 64 - config.pt_offset_bit);
        virt_page_num = stoi(virt_page_num_bin, 0, 2);
        if (virt_page_num >= config.virtual_page_count) {
          cout << "hierarchy: virtual address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        cout << hex << setw(8) << setfill('0') << temp;
        cout << setfill(' ') << setw(7) << hex << virt_page_num << " ";

        // PAGE OFFSET CALCULATION AND PRINTING
        page_off_bin = bin_string.substr(64 - config.pt_offset_bit);
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << setfill(' ') << page_off;

        tlb_index_and_tag = tlb_index_tag_getter(virt_page_num, config.dtlb_set_count);
        // cout << "tlb_index = " << tlb_index_and_tag.first << " tlb_tag = " << tlb_index_and_tag.second << " ";
        tlb_index = tlb_index_and_tag.first;
        tlb_tag = tlb_index_and_tag.second;
        cout << setw(7) << setfill(' ') << hex << tlb_tag;
        cout << setw(4) << setfill(' ') << hex << tlb_index;
        if (translation_buffer.check_dtlb(tlb_index, tlb_tag, config.counter)) {
          cout << setw(4) << setfill(' ') << " hit ";
          cout << setw(5) << setfill(' ') << " ";
          dtlb_hits += 1;
        } else {
          cout << setw(4) << setfill(' ') << " miss";
          dtlb_misses += 1;
          pfn = PAGE_TABLE.check_page_table(virt_page_num);
          // cout << " " << PAGE_TABLE.page_table[virt_page_num].valid << "<-isvalid";
          if (pfn == -1) {
            pfn = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE).first;
            // cout << "wasnt valid so insert with pfn value->" << pfn << " pfn used count=" << PAGE_TABLE.pfn_used_count << " ";
            // page_was_dirty = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read).second;
            pt_faults += 1;
            pt_res = "miss";
            pt_refs += 1;
            disk_refs += 1;
          } else {
            pt_refs += 1;
            pt_hits += 1;
            pt_res = "hit ";
          }
          translation_buffer.insert_to_dtlb(tlb_index, tlb_tag, config.counter, pfn);
          cout << setw(5) << setfill(' ') << pt_res;
        }
        /*
        // PAGE TABLE RESULT PRINTING
        pfn = PAGE_TABLE.check_page_table(virt_page_num);
        // cout << " " << PAGE_TABLE.page_table[virt_page_num].valid << "<-isvalid";
        if (pfn == -1) {
          pfn = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE).first;
          // cout << "wasnt valid so insert with pfn value->" << pfn << " pfn used count=" << PAGE_TABLE.pfn_used_count << " ";
          // page_was_dirty = PAGE_TABLE.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, !is_read).second;
          pt_faults += 1;
          pt_res = "miss";
          pt_refs += 1;
          disk_refs += 1;
        } else {
          pt_refs += 1;
          pt_hits += 1;
          pt_res = "hit ";
        }
        */
        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = PAGE_TABLE.vpn_to_phys_address(virt_page_num, page_off, config.pt_offset_bit, config.virtual_page_count, config.physical_page_count, config.counter, !is_read, DATA_CACHE, L2_CACHE);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        phys_page_num_bin = p_bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = p_bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = p_bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        dc_index = stoi(dc_index_bin, 0, 2);
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;

        // check data cache for block, if hit no need to insert
        if (DATA_CACHE.check_cache(dc_index, dc_tag, config.counter, !is_read, -1, false)) {
          // datacache was found but if it was a write we need to also update the l2 dirty bit & access time
          // hit
          if (!is_read) { // hit write
            l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            l2_index = stoi(l2_index_bin, 0, 2);
            L2_CACHE.update_dirty_bit(l2_index, l2_tag, config.counter);
            // l2_hits += 1; weird imo, wouldnt this be accessessing and hitting the l2 ?
            config.counter += 1;
          }
          dc_hits += 1;
          cout << " hit ";
          if (is_read) {
            cout << " \n";
            continue;
          }
        } else {
          if (is_read) {
            DATA_CACHE.insert_to_cache(dc_index, dc_tag, config.counter, dirty_bit, pfn, phys_page_num_bin);
          }
          dc_misses += 1;
          if (config.l2_enabled) {
            cout << " miss";
          } else {
            cout << " miss ";
          }
        }
        if (!config.l2_enabled) {
          cout << "\n";
        } else {
          l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
          l2_tag = stoi(l2_tag_bin, 0, 2);
          cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";

          // L2 INDEX CALC & PRINT
          l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
          l2_index = stoi(l2_index_bin, 0, 2);
          cout << setw(3) << setfill(' ') << hex << l2_index;
          was_l2_full = L2_CACHE.check_if_index_is_full(l2_index);
          if (L2_CACHE.check_l2(l2_index, l2_tag, config.counter, dirty_bit, pfn, false, dc_index, dc_tag)) {
            cout << " hit \n";
            l2_hits += 1;
          } else {
            l2_misses += 1;
            pair<bool, string> was_l2_replaced_and_if_yes_dc_phys_address = L2_CACHE.insert_to_l2(l2_index, l2_tag, config.counter, dirty_bit, pfn, dc_index, dc_tag, p_bin_string, memory_refs);
            // the insert to l2 function will fully determine if mem ref incrememnt as it checks in the function for the dirty bit
            bool was_there_an_l2_eviction = was_l2_replaced_and_if_yes_dc_phys_address.first;
            if (was_there_an_l2_eviction) {
              string replace_address = was_l2_replaced_and_if_yes_dc_phys_address.second;
              dc_tag_bin = replace_address.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
              dc_tag = stoi(dc_tag_bin, 0, 2);
              dc_index_bin = replace_address.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
              dc_index = stoi(dc_index_bin, 0, 2);
              DATA_CACHE.evict_given_l2_phys_address(dc_index, dc_tag);
              // dc never is dirty as any write writes to l2
            }
            memory_refs += 1;
            cout << " miss\n";
          }
        }   
      }
    }
  }
  print_stats(dtlb_hits, dtlb_misses, pt_hits, pt_faults, dc_hits, dc_misses, l2_hits, l2_misses, total_reads, total_writes, memory_refs, pt_refs, disk_refs);
}

void print_stats(double dtlb_hits, double dtlb_misses, double pt_hits, double pt_faults, double dc_hits, double dc_misses, double l2_hits, double l2_misses, double total_reads, double total_writes, double mem_refs, double pt_refs, double disk_refs) {
  cout << "\nSimulation statistics\n\n";
  cout << "dtlb hits        : " << dtlb_hits << "\n";
  cout << "dtlb misses      : " << dtlb_misses << "\n";
  if (dtlb_misses == 0 && dtlb_hits == 0) {
    cout << "dtlb hit ratio   : N/A\n\n";
  } else if (dtlb_misses == 0 && dtlb_hits != 0) {
    cout << "dtlb hit ratio   : 1\n\n";
  } else {
    double dtlb_ratio = dtlb_hits / (dtlb_misses + dtlb_hits);
    cout << "dtlb hit ratio   : " << fixed << setprecision(6) << dtlb_ratio << "\n\n";
  }

  cout << "pt hits          : " << defaultfloat << pt_hits << "\n";
  cout << "pt faults        : " << pt_faults << "\n";
  if (pt_faults == 0 && pt_hits == 0) {
    cout << "pt hit ratio     : N/A\n\n";
  } else if (pt_faults == 0 && pt_hits != 0) {
    cout << "pt hit ratio     : 1\n\n";
  } else {
    double pt_ratio = pt_hits / (pt_faults + pt_hits);
    cout << "pt hit ratio     : " << fixed << setprecision(6) << pt_ratio << "\n\n";
  }

  cout << "dc hits          : " << defaultfloat << dc_hits << "\n";
  cout << "dc misses        : " << dc_misses << "\n";
  if (dc_hits == 0 && dc_misses == 0) {
    cout << "dc hit ratio     : N/A\n\n";
  } else if (dc_misses == 0 && dc_hits != 0) {
    cout << "dc hit ratio     : 1\n\n";
  } else {
    double dc_ratio = dc_hits / (dc_misses + dc_hits);
    cout << "dc hit ratio     : " << fixed << setprecision(6) << dc_ratio << "\n\n";
  }

  cout << "L2 hits          : " << defaultfloat << l2_hits << "\n";
  cout << "L2 misses        : " << l2_misses << "\n";
  if (l2_hits == 0 && l2_misses == 0) {
    cout << "L2 hit ratio     : N/A\n\n";
  } else if (l2_hits != 0 && l2_misses == 0) {
    cout << "L2 hit ratio     : 1\n\n";
  } else {
    double l2_ratio = l2_hits / (l2_misses + l2_hits);
    cout << "L2 hit ratio     : " << fixed << setprecision(6) << l2_ratio << "\n\n";
  }

  cout << "Total reads      : " << defaultfloat << total_reads << "\n";
  cout << "Total writes     : " << total_writes << "\n";
  double read_ratio = total_reads / (total_writes + total_reads);
  cout << "Ratio of reads   : " << fixed << setprecision(6) << read_ratio << "\n\n";

  cout << "main memory refs : " << defaultfloat << mem_refs << "\n";
  cout << "page table refs  : " << pt_refs << "\n";
  cout << "disk refs        : " << disk_refs << "\n";
}

pair<int, int> tlb_index_tag_getter(int vpn, int tlb_set_count) {
  bitset<64> v(vpn);
  int tlb_index_bits = log2(tlb_set_count);
  string vpn_str = v.to_string();
  string ind_str = vpn_str.substr(64 - tlb_index_bits);
  int index = stoi(ind_str, 0, 2);
  string tag_str = vpn_str.substr(0, 64 - tlb_index_bits);
  int tag = stoi(tag_str, 0, 2);
  return {index, tag};
}
