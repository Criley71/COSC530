#include "config.h"
#include "dc.h"
#include "dtlb.h"
#include "l2.h"
#include "pt.h"
#include <bitset>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>

using namespace std;

// safe substring for bitstrings: checks bounds and clamps length to avoid std::out_of_range

void read_data_no_virtual(Config config); // reads in the line and will call the appropraite set of functions based on the config
void read_data_virtual(Config config);

void simulate_withOUT_l2_enable(Config config);
void simulate_with_l2_enabled(Config config);

void physical_write_back_dc_no_l2(Config config, string address);
void physical_write_back_dc_with_l2(Config config, string address);
void print_stats(double dtlb_hits, double dtlb_misses, double pt_hits, double pt_faults, double dc_hits, double dc_misses, double l2_hits, double l2_misses, double total_reads, double total_writes, double mem_refs, double pt_refs, double disk_refs);
string substr_safe(const string &s, int pos, int len);
pair<int, int> dtlb_index_tag_getter(int vpn, int tlb_set_count);
int main() {
  Config config = Config();
  config.print_config_after_read();
  if (!config.l2_enabled) {
    simulate_withOUT_l2_enable(config);
  } else {
    simulate_with_l2_enabled(config);
  }
}

void simulate_withOUT_l2_enable(Config config) {
  bool is_write;
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
  string line;
  string hex_val;
  stringstream ss;
  bitset<64> b;
  bitset<64> p;

  string bin_string;
  string phys_page_num_bin;
  long phys_page_num;
  string page_off_bin;
  int page_off;
  string dc_tag_bin;
  int dc_tag;
  string dc_index_bin;
  int dc_index;

  string virt_page_num_bin;
  int virt_page_num;
  int pfn;
  string pt_res;
  string p_bin_string;

  int dtlb_index;
  int dtlb_tag;
  pair<int, int> dtlb_index_and_tag;
  pair<bool, int> dtlb_search_result;
  DC dc = DC(config.dc_set_count, config.dc_set_size, config.dc_line_size, config.dc_write_thru_no_allo, config.dc_index_bits, config.dc_offset_bits);
  PT pt = PT(config.virtual_page_count, config.pt_to_l2_ratio, config.physical_page_count);
  // only have l2 since it is a pt check parameter
  L2 l2 = L2(config.l2_set_count, config.l2_set_size, config.l2_line_size, config.l2_write_thru_no_allo, config.l2_index_bits, config.l2_offset_bits, config.l2_to_dc_ratio);
  DTLB dtlb = DTLB(config.dtlb_set_count, config.dtlb_set_size);

  while (getline(cin, line)) {
    config.counter += 1;
    if (line[0] == 'R') {
      is_write = false;
      total_reads += 1;
    } else if (line[0] == 'W') {
      is_write = true;
      total_writes += 1;
    }
    hex_val = line.substr(2, line.size());
    ss << hex << hex_val;
    unsigned temp;
    ss >> temp;
    ss.clear();
    b = bitset<64>(temp);
    bin_string = b.to_string();
    config.counter += 1;
    if (!config.dc_write_thru_no_allo) {
      if (!config.virt_addr) { // no virtual address
        phys_page_num_bin = bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2); // physical page number calculations
        if (phys_page_num >= config.physical_page_count) {
          cout << "hierarchy: physical address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        // print out address and basic stuff
        cout << hex << setw(8) << setfill('0') << temp << "  ";
        cout << setw(6) << setfill(' ') << " ";
        page_off_bin = bin_string.substr(64 - (config.pt_offset_bit));
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << page_off << "  ";
        cout << " " << setw(6) << setfill(' ');
        cout << " ";
        cout << " " << setw(3) << setfill(' ');
        cout << " ";
        cout << " " << setw(4) << setfill(' ');
        cout << " ";
        cout << setw(4) << setfill(' ');
        cout << " ";
        cout << setw(4) << setfill(' ') << hex << phys_page_num;
        // dc calculations
        dc_tag_bin = bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;
        dc_index_bin = bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index << " ";
        // check if the block is in the data cache
        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, -1, false)) {
          cout << setfill(' ') << setw(4) << "hit  \n";
          dc_hits += 1;
          config.counter += 1; // if so then its a dc hit
        } else {
          // was not found and need to be inserted.
          // check if the evicted block was dirty
          pair<bool, string> was_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, -1, bin_string);
          if (was_replace.first) {
            memory_refs += 1; // the replaced block was dirty so write to memory
          }
          dc_misses += 1; // its a dc miss and need to grab block from memory
          memory_refs += 1;
          cout << setfill(' ') << setw(4) << "miss \n";
          config.counter += 1;
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

        page_off_bin = bin_string.substr(64 - config.pt_offset_bit);
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << setfill(' ') << page_off;

        pfn = pt.check_page_table(virt_page_num, config.counter, is_write);
        if (pfn == -1) {
          pfn = pt.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, is_write, page_off, config.pt_offset_bit, config.l2_enabled, dc, l2, disk_refs, memory_refs, l2_hits, pt_refs, config.dtlb_enabled, dtlb).second;
          pt_faults += 1;
          pt_refs += 1;
          disk_refs += 1;
          pt_res = "miss";
        } else {
          pt_refs += 1;
          pt_hits += 1;
          pt_res = "hit ";
        }
        cout << setw(21) << setfill(' ') << pt_res;

        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = pt.translate_to_phys_address(pfn, page_off, config.pt_offset_bit);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        // cout << " ->" << config.pt_offset_bit << "<- ";
        phys_page_num_bin = substr_safe(p_bin_string, 0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = substr_safe(p_bin_string, 0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = substr_safe(p_bin_string, (64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;

        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << " hit  \n";
          dc_hits += 1;
          config.counter += 1;
          continue;
        } else {
          pair<bool, string> was_dc_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          dc_misses += 1;
          memory_refs += 1;
          if (was_dc_replace.first) {
            memory_refs += 1; // replaced block was dirty
          }
          cout << setfill(' ') << setw(4) << " miss \n";
          config.counter += 1;
        }
      } else if (config.virt_addr && config.dtlb_enabled) {
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

        dtlb_index_and_tag = dtlb_index_tag_getter(virt_page_num, config.dtlb_set_count);
        dtlb_index = dtlb_index_and_tag.first;
        dtlb_tag = dtlb_index_and_tag.second;
        cout << setw(7) << setfill(' ') << hex << dtlb_tag;
        cout << setw(4) << setfill(' ') << hex << dtlb_index;

        dtlb_search_result = dtlb.check_dtlb(dtlb_index, dtlb_tag, config.counter);
        if (dtlb_search_result.first) {
          cout << setw(4) << setfill(' ') << " hit ";
          cout << setw(5) << setfill(' ') << " ";
          dtlb_hits += 1;
          pfn = dtlb_search_result.second;
          config.counter += 1;
          pt.check_page_table(virt_page_num, config.counter, is_write);
        } else {
          cout << setw(4) << setfill(' ') << " miss";
          dtlb_misses += 1;
          pfn = pt.check_page_table(virt_page_num, config.counter, is_write);
          if (pfn == -1) {
            pfn = pt.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, is_write, page_off, config.pt_offset_bit, config.l2_enabled, dc, l2, disk_refs, memory_refs, l2_hits, pt_refs, config.dtlb_enabled, dtlb).second;
            pt_faults += 1;
            pt_refs += 1;
            disk_refs += 1;
            pt_res = "miss";
          } else {
            pt_refs += 1;
            pt_hits += 1;
            pt_res = "hit ";
          }
          dtlb.insert_to_dtlb(dtlb_index, dtlb_tag, config.counter, pfn);
          cout << setw(5) << setfill(' ') << pt_res;
        }

        int physical_address = pt.translate_to_phys_address(pfn, page_off, config.pt_offset_bit);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        // cout << " ->" << config.pt_offset_bit << "<- ";
        // cout << "||||| " << p_bin_string << " ||||  ";
        phys_page_num_bin = substr_safe(p_bin_string, 0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = substr_safe(p_bin_string, 0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = substr_safe(p_bin_string, (64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;

        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << " hit  \n";
          dc_hits += 1;
          config.counter += 1;
          continue;
        } else {
          pair<bool, string> was_dc_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          dc_misses += 1;
          memory_refs += 1;
          if (was_dc_replace.first) {
            memory_refs += 1; // replaced block was dirty
          }
          cout << setfill(' ') << setw(4) << " miss \n";
          config.counter += 1;
        }
      }
      config.counter += 1;
    } else if (config.dc_write_thru_no_allo) {
      if (!config.virt_addr) {
        phys_page_num_bin = bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2); // physical page number calculations
        if (phys_page_num >= config.physical_page_count) {
          cout << "hierarchy: physical address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        // print out address and basic stuff
        cout << hex << setw(8) << setfill('0') << temp << "  ";
        cout << setw(6) << setfill(' ') << " ";
        page_off_bin = bin_string.substr(64 - (config.pt_offset_bit));
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << page_off << "  ";
        cout << " " << setw(6) << setfill(' ');
        cout << " ";
        cout << " " << setw(3) << setfill(' ');
        cout << " ";
        cout << " " << setw(4) << setfill(' ');
        cout << " ";
        cout << setw(4) << setfill(' ');
        cout << " ";
        cout << setw(4) << setfill(' ') << hex << phys_page_num;
        // dc calculations
        dc_tag_bin = bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;
        dc_index_bin = bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index << " ";
        // check if the block is in the data cache
        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, -1, false)) {
          cout << setfill(' ') << setw(4) << "hit  \n";
          dc_hits += 1;
          config.counter += 1; // if so then its a dc hit
          if (is_write) {
            memory_refs += 1;
          }
        } else {
          cout << setfill(' ') << setw(4) << "miss \n";
          config.counter += 1;
          dc_misses += 1;
          memory_refs;
          if (!is_write) {
            pair<bool, string> was_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, -1, bin_string);
          }
          memory_refs += 1; // cant be dirty so a miss means that we only grab the block from memory
          // dont have to write back a dirty block
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

        page_off_bin = bin_string.substr(64 - config.pt_offset_bit);
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << setfill(' ') << page_off;

        pfn = pt.check_page_table(virt_page_num, config.counter, is_write);
        if (pfn == -1) {
          pfn = pt.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, is_write, page_off, config.pt_offset_bit, config.l2_enabled, dc, l2, disk_refs, memory_refs, l2_hits, pt_refs, config.dtlb_enabled, dtlb).second;
          pt_faults += 1;
          pt_refs += 1;
          disk_refs += 1;
          pt_res = "miss";
        } else {
          pt_refs += 1;
          pt_hits += 1;
          pt_res = "hit ";
        }
        cout << setw(21) << setfill(' ') << pt_res;

        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = pt.translate_to_phys_address(pfn, page_off, config.pt_offset_bit);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        // cout << " ->" << config.pt_offset_bit << "<- ";
        phys_page_num_bin = substr_safe(p_bin_string, 0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = substr_safe(p_bin_string, 0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = substr_safe(p_bin_string, (64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;

        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << " hit  \n";
          dc_hits += 1;
          config.counter += 1; // if so then its a dc hit
          if (is_write) {
            memory_refs += 1;
          }
        } else {
          cout << setfill(' ') << setw(4) << " miss \n";
          config.counter += 1;
          dc_misses += 1;
          memory_refs;
          if (!is_write) {
            pair<bool, string> was_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          }
          memory_refs += 1; // cant be dirty so a miss means that we only grab the block from memory
          // dont have to write back a dirty block
        }
      } else if (config.virt_addr && config.dtlb_enabled) {
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

        dtlb_index_and_tag = dtlb_index_tag_getter(virt_page_num, config.dtlb_set_count);
        dtlb_index = dtlb_index_and_tag.first;
        dtlb_tag = dtlb_index_and_tag.second;
        cout << setw(7) << setfill(' ') << hex << dtlb_tag;
        cout << setw(4) << setfill(' ') << hex << dtlb_index;

        dtlb_search_result = dtlb.check_dtlb(dtlb_index, dtlb_tag, config.counter);
        if (dtlb_search_result.first) {
          cout << setw(4) << setfill(' ') << " hit ";
          cout << setw(5) << setfill(' ') << " ";
          dtlb_hits += 1;
          pfn = dtlb_search_result.second;
          config.counter += 1;
          pt.check_page_table(virt_page_num, config.counter, is_write);
        } else {
          cout << setw(4) << setfill(' ') << " miss";
          dtlb_misses += 1;
          pfn = pt.check_page_table(virt_page_num, config.counter, is_write);
          if (pfn == -1) {
            pfn = pt.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, is_write, page_off, config.pt_offset_bit, config.l2_enabled, dc, l2, disk_refs, memory_refs, l2_hits, pt_refs, config.dtlb_enabled, dtlb).second;
            pt_faults += 1;
            pt_refs += 1;
            disk_refs += 1;
            pt_res = "miss";
          } else {
            pt_refs += 1;
            pt_hits += 1;
            pt_res = "hit ";
          }
          dtlb.insert_to_dtlb(dtlb_index, dtlb_tag, config.counter, pfn);
          cout << setw(5) << setfill(' ') << pt_res;
        }

        int physical_address = pt.translate_to_phys_address(pfn, page_off, config.pt_offset_bit);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        // cout << " ->" << config.pt_offset_bit << "<- ";
        // cout << "||||| " << p_bin_string << " ||||  ";
        phys_page_num_bin = substr_safe(p_bin_string, 0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = substr_safe(p_bin_string, 0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = substr_safe(p_bin_string, (64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;

        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << " hit  \n";
          dc_hits += 1;
          config.counter += 1; // if so then its a dc hit
          if (is_write) {
            memory_refs += 1;
          }
        } else {
          cout << setfill(' ') << setw(4) << " miss \n";
          config.counter += 1;
          dc_misses += 1;
          memory_refs;
          if (!is_write) {
            pair<bool, string> was_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          }
          memory_refs += 1; // cant be dirty so a miss means that we only grab the block from memory
          // dont have to write back a dirty block
        }
      }
    }
  }
  print_stats(dtlb_hits, dtlb_misses, pt_hits, pt_faults, dc_hits, dc_misses, l2_hits, l2_misses, total_reads, total_writes, memory_refs, pt_refs, disk_refs);
}

void simulate_with_l2_enabled(Config config) {
  bool is_write;
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
  string line;
  string hex_val;
  stringstream ss;
  bitset<64> b;
  bitset<64> p;

  string bin_string;
  string phys_page_num_bin;
  int phys_page_num;
  string page_off_bin;
  int page_off;
  string dc_tag_bin;
  int dc_tag;
  string dc_index_bin;
  int dc_index;
  int l2_tag;
  string l2_tag_bin;
  int l2_index;
  string l2_index_bin;

  string virt_page_num_bin;
  int virt_page_num;
  int pfn;
  string pt_res;
  string p_bin_string;

  int dtlb_index;
  int dtlb_tag;
  pair<int, int> dtlb_index_and_tag;
  pair<bool, int> dtlb_search_result;

  DC dc(config.dc_set_count, config.dc_set_size, config.dc_line_size, config.dc_write_thru_no_allo, config.dc_index_bits, config.dc_offset_bits);
  L2 l2(config.l2_set_count, config.l2_set_size, config.l2_line_size, config.l2_write_thru_no_allo, config.l2_index_bits, config.l2_offset_bits, config.l2_to_dc_ratio);
  PT pt = PT(config.virtual_page_count, config.pt_to_l2_ratio, config.physical_page_count);
  DTLB dtlb = DTLB(config.dtlb_set_count, config.dtlb_set_size);
  while (getline(cin, line)) {
    config.counter += 1; // i dont remember why i made this part of the config class but its too late to change
    if (line[0] == 'R') {
      is_write = false;
      total_reads += 1;
    } else if (line[0] == 'W') {
      is_write = true;
      total_writes += 1;
    }
    hex_val = line.substr(2, line.size());
    ss << hex << hex_val;
    unsigned temp;
    ss >> temp;
    ss.clear();
    b = bitset<64>(temp);
    bin_string = b.to_string();

    if (!config.dc_write_thru_no_allo && !config.l2_write_thru_no_allo) {
      if (!config.virt_addr) { // No virtual address enabled
        // begin calculating basic information and make sure address is valid
        phys_page_num_bin = bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        if (phys_page_num >= config.physical_page_count) {
          cout << "hierarchy: physical address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        // print out addres, page offset and page num
        cout << hex << setw(8) << setfill('0') << temp << "  ";
        cout << setw(6) << setfill(' ') << " ";
        page_off_bin = bin_string.substr(64 - (config.pt_offset_bit));
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << page_off << "  ";
        cout << " " << setw(6) << setfill(' ');
        cout << " ";
        cout << " " << setw(3) << setfill(' ');
        cout << " ";
        cout << " " << setw(4) << setfill(' ');
        cout << " ";
        cout << setw(4) << setfill(' ');
        cout << " ";
        cout << setw(4) << setfill(' ') << hex << phys_page_num;
        // start calculating dc stuff
        dc_tag_bin = bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;
        dc_index_bin = bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index << " ";
        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, -1, false)) {
          cout << setfill(' ') << setw(4) << " hit\n";
          dc_hits += 1;
          continue;
          // address was found in dc
        } else {
          // address wasnt found. need to check if a block was evicted to then see if it was dirty and write that to l2
          pair<bool, string> was_dc_replaced_and_was_dirty = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, -1, bin_string);
          dc_misses += 1;

          cout << setfill(' ') << setw(4) << "miss";
          bool dirty_replaced = was_dc_replaced_and_was_dirty.first;
          if (dirty_replaced) {
            l2_hits += 1; // if it was dirty then we hit the l2 block and update it
            string old_address = was_dc_replaced_and_was_dirty.second;
            l2_tag_bin = old_address.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = old_address.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter); // update only the dirty bit and access time
            config.counter += 1;
          }
        }
        // start l2 calculations
        l2_tag_bin = bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
        l2_tag = stoi(l2_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";
        l2_index_bin = bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
        if (config.l2_index_bits == 0) {
          l2_index = 0;
        } else {
          l2_index = stoi(l2_index_bin, 0, 2);
        }
        cout << setw(3) << setfill(' ') << hex << l2_index;
        // check if the block was in l2
        if (l2.check_l2(l2_index, l2_tag, config.counter, is_write, -1, false)) {
          l2_hits += 1; // was found so l2 hit
          cout << " hit \n";
          config.counter += 1;
        } else {
          // was not found, so l2 miss and need to reference memory to get it.
          cout << " miss\n";
          memory_refs += 1;
          l2_misses += 1;
          int temp_mem_refs = memory_refs;
          // so basically we need this temp counter as the insert and invalidate functions will increment them but
          // we dont want to double count ie a dirty dc is invalidated but the l2 was already dirty so it should just be 1
          // memory access we can incremement the temp in the dc eviction function and the normal in the l2 insert
          // we then take the larger of the 2
          pair<bool, vector<pair<int, int>>> check_for_l2_evict = l2.insert_to_l2(l2_index, l2_tag, config.counter, is_write, -1, memory_refs, dc_index, dc_tag);
          if (check_for_l2_evict.first) { // go through the invalidated dc blocks due to l2 eviction and invalidate them
            vector<pair<int, int>> dc_index_and_tags_to_invalidate = check_for_l2_evict.second;
            for (int i = 0; i < dc_index_and_tags_to_invalidate.size(); i++) {
              if (dc_index_and_tags_to_invalidate[i].first == -1) {
                continue;
              }
              dc.invalidate_bc_l2_eviction(dc_index_and_tags_to_invalidate[i].first, dc_index_and_tags_to_invalidate[i].second, l2_hits, temp_mem_refs);
            }
            if (temp_mem_refs >= memory_refs) {
              memory_refs = temp_mem_refs;
            }
            config.counter += 1;
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

        page_off_bin = bin_string.substr(64 - config.pt_offset_bit);
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << setfill(' ') << page_off;

        pfn = pt.check_page_table(virt_page_num, config.counter, is_write);
        if (pfn == -1) {
          pfn = pt.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, is_write, page_off, config.pt_offset_bit, config.l2_enabled, dc, l2, disk_refs, memory_refs, l2_hits, pt_refs, config.dtlb_enabled, dtlb).second;
          pt_faults += 1;
          pt_refs += 1;
          disk_refs += 1;
          pt_res = "miss";
        } else {
          pt_refs += 1;
          pt_hits += 1;
          pt_res = "hit ";
        }
        cout << setw(21) << setfill(' ') << pt_res;

        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = pt.translate_to_phys_address(pfn, page_off, config.pt_offset_bit);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        // cout << " ->" << config.pt_offset_bit << "<- ";
        phys_page_num_bin = substr_safe(p_bin_string, 0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = substr_safe(p_bin_string, 0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = substr_safe(p_bin_string, (64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;
        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << " hit  \n";
          dc_hits += 1;
          config.counter += 1;

          continue;
        } else {
          pair<bool, string> was_dc_replaced_and_was_dirty = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          dc_misses += 1;
          if (was_dc_replaced_and_was_dirty.first) {
            l2_hits += 1; // replaced block was dirty maybe dont need
            string old_address = was_dc_replaced_and_was_dirty.second;
            l2_tag_bin = old_address.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = old_address.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter); // update only the dirty bit and access time
            config.counter += 1;
          }
          cout << setfill(' ') << setw(4) << " miss";
          config.counter += 1;
        }

        l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
        l2_tag = stoi(l2_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";
        l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
        if (config.l2_index_bits == 0) {
          l2_index = 0;
        } else {
          l2_index = stoi(l2_index_bin, 0, 2);
        }
        cout << setw(3) << setfill(' ') << hex << l2_index;
        if (l2.check_l2(l2_index, l2_tag, config.counter, is_write, pfn, false)) {
          l2_hits += 1; // was found so l2 hit
          cout << " hit \n";
          config.counter += 1;
        } else {
          // was not found, so l2 miss and need to reference memory to get it.
          cout << " miss\n";
          memory_refs += 1;
          l2_misses += 1;
          int temp_mem_refs = memory_refs;
          // so basically we need this temp counter as the insert and invalidate functions will increment them but
          // we dont want to double count ie a dirty dc is invalidated but the l2 was already dirty so it should just be 1
          // memory access we can incremement the temp in the dc eviction function and the normal in the l2 insert
          // we then take the larger of the 2
          // cout << " pfn for l2 " << pfn << "||";
          pair<bool, vector<pair<int, int>>> check_for_l2_evict = l2.insert_to_l2(l2_index, l2_tag, config.counter, is_write, pfn, memory_refs, dc_index, dc_tag);
          if (check_for_l2_evict.first) { // go through the invalidated dc blocks due to l2 eviction and invalidate them
            vector<pair<int, int>> dc_index_and_tags_to_invalidate = check_for_l2_evict.second;
            for (int i = 0; i < dc_index_and_tags_to_invalidate.size(); i++) {
              if (dc_index_and_tags_to_invalidate[i].first == -1) {
                continue;
              }
              dc.invalidate_bc_l2_eviction(dc_index_and_tags_to_invalidate[i].first, dc_index_and_tags_to_invalidate[i].second, l2_hits, memory_refs);
            }
            memory_refs = max(memory_refs, temp_mem_refs);
            config.counter += 1;
          }
        }
      } else if (config.virt_addr && config.dtlb_enabled) {
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

        dtlb_index_and_tag = dtlb_index_tag_getter(virt_page_num, config.dtlb_set_count);
        dtlb_index = dtlb_index_and_tag.first;
        dtlb_tag = dtlb_index_and_tag.second;
        cout << setw(7) << setfill(' ') << hex << dtlb_tag;
        cout << setw(4) << setfill(' ') << hex << dtlb_index;

        dtlb_search_result = dtlb.check_dtlb(dtlb_index, dtlb_tag, config.counter);
        if (dtlb_search_result.first) {
          cout << setw(4) << setfill(' ') << " hit ";
          cout << setw(5) << setfill(' ') << " ";
          dtlb_hits += 1;
          pfn = dtlb_search_result.second;
          config.counter += 1;
          pt.check_page_table(virt_page_num, config.counter, is_write);
        } else {
          cout << setw(4) << setfill(' ') << " miss";
          dtlb_misses += 1;
          pfn = pt.check_page_table(virt_page_num, config.counter, is_write);
          if (pfn == -1) {
            pfn = pt.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, is_write, page_off, config.pt_offset_bit, config.l2_enabled, dc, l2, disk_refs, memory_refs, l2_hits, pt_refs, config.dtlb_enabled, dtlb).second;
            pt_faults += 1;
            pt_refs += 1;
            disk_refs += 1;
            pt_res = "miss";
          } else {
            pt_refs += 1;
            pt_hits += 1;
            pt_res = "hit ";
          }
          dtlb.insert_to_dtlb(dtlb_index, dtlb_tag, config.counter, pfn);
          cout << setw(5) << setfill(' ') << pt_res;
        }

        int physical_address = pt.translate_to_phys_address(pfn, page_off, config.pt_offset_bit);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        // cout << " ->" << config.pt_offset_bit << "<- ";
        // cout << "||||| " << p_bin_string << " ||||  ";
        phys_page_num_bin = substr_safe(p_bin_string, 0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = substr_safe(p_bin_string, 0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;

        // DC INDEX FOR VIRTUAL ADDRESSES
        dc_index_bin = substr_safe(p_bin_string, (64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index;
        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << " hit  \n";
          dc_hits += 1;
          config.counter += 1;
          continue;
        } else {
          pair<bool, string> was_dc_replaced_and_was_dirty = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          dc_misses += 1;
          if (was_dc_replaced_and_was_dirty.first) {
            l2_hits += 1; // replaced block was dirty maybe dont need
            string old_address = was_dc_replaced_and_was_dirty.second;
            l2_tag_bin = old_address.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = old_address.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter); // update only the dirty bit and access time
            config.counter += 1;
          }
          cout << setfill(' ') << setw(4) << " miss";
          config.counter += 1;
        }

        l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
        l2_tag = stoi(l2_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";
        l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
        if (config.l2_index_bits == 0) {
          l2_index = 0;
        } else {
          l2_index = stoi(l2_index_bin, 0, 2);
        }
        cout << setw(3) << setfill(' ') << hex << l2_index;
        if (l2.check_l2(l2_index, l2_tag, config.counter, is_write, pfn, false)) {
          l2_hits += 1; // was found so l2 hit
          cout << " hit \n";
          config.counter += 1;
        } else {
          // was not found, so l2 miss and need to reference memory to get it.
          cout << " miss\n";
          memory_refs += 1;
          l2_misses += 1;
          int temp_mem_refs = memory_refs;
          // so basically we need this temp counter as the insert and invalidate functions will increment them but
          // we dont want to double count ie a dirty dc is invalidated but the l2 was already dirty so it should just be 1
          // memory access we can incremement the temp in the dc eviction function and the normal in the l2 insert
          // we then take the larger of the 2
          // cout << " pfn for l2 " << pfn << "||";
          pair<bool, vector<pair<int, int>>> check_for_l2_evict = l2.insert_to_l2(l2_index, l2_tag, config.counter, is_write, pfn, memory_refs, dc_index, dc_tag);
          if (check_for_l2_evict.first) { // go through the invalidated dc blocks due to l2 eviction and invalidate them
            vector<pair<int, int>> dc_index_and_tags_to_invalidate = check_for_l2_evict.second;
            for (int i = 0; i < dc_index_and_tags_to_invalidate.size(); i++) {
              if (dc_index_and_tags_to_invalidate[i].first == -1) {
                continue;
              }
              dc.invalidate_bc_l2_eviction(dc_index_and_tags_to_invalidate[i].first, dc_index_and_tags_to_invalidate[i].second, l2_hits, temp_mem_refs);
            }
            memory_refs = max(memory_refs, temp_mem_refs);
            config.counter += 1;
          }
        }
      }
    } else if (config.dc_write_thru_no_allo && !config.l2_write_thru_no_allo) {
      if (!config.virt_addr) { // No virtual address enabled
        // begin calculating basic information and make sure address is valid
        phys_page_num_bin = bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        if (phys_page_num >= config.physical_page_count) {
          cout << "hierarchy: physical address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        // print out addres, page offset and page num
        cout << hex << setw(8) << setfill('0') << temp << "  ";
        cout << setw(6) << setfill(' ') << " ";
        page_off_bin = bin_string.substr(64 - (config.pt_offset_bit));
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << page_off << "  ";
        cout << " " << setw(6) << setfill(' ');
        cout << " ";
        cout << " " << setw(3) << setfill(' ');
        cout << " ";
        cout << " " << setw(4) << setfill(' ');
        cout << " ";
        cout << setw(4) << setfill(' ');
        cout << " ";
        cout << setw(4) << setfill(' ') << hex << phys_page_num;
        // start calculating dc stuff
        dc_tag_bin = bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;
        dc_index_bin = bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index << " ";
        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << "hit ";
          dc_hits += 1;
          config.counter += 1; // if so then its a dc hit
          if (is_write) {
            l2_tag_bin = bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter);
          }
          if (!is_write) {
            cout << " \n";
            continue;
            // memory_refs += 1;
          }

        } else {
          cout << setfill(' ') << setw(4) << "miss";
          config.counter += 1;
          dc_misses += 1;
          // memory_refs;
          if (!is_write) {
            pair<bool, string> was_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          }
          if (is_write) {
            l2_tag_bin = bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter);
          }
          // memory_refs += 1; // cant be dirty so a miss means that we only grab the block from memory
          //  dont have to write back a dirty block
        }
        // start l2 calculations
        l2_tag_bin = bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
        l2_tag = stoi(l2_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";
        l2_index_bin = bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
        if (config.l2_index_bits == 0) {
          l2_index = 0;
        } else {
          l2_index = stoi(l2_index_bin, 0, 2);
        }
        cout << setw(3) << setfill(' ') << hex << l2_index;
        // check if the block was in l2
        if (l2.check_l2(l2_index, l2_tag, config.counter, is_write, -1, false)) {
          l2_hits += 1; // was found so l2 hit
          cout << " hit \n";
          config.counter += 1;
        } else {
          // was not found, so l2 miss and need to reference memory to get it.
          cout << " miss\n";
          memory_refs += 1;
          l2_misses += 1;
          int temp_mem_refs = memory_refs;
          // so basically we need this temp counter as the insert and invalidate functions will increment them but
          // we dont want to double count ie a dirty dc is invalidated but the l2 was already dirty so it should just be 1
          // memory access we can incremement the temp in the dc eviction function and the normal in the l2 insert
          // we then take the larger of the 2
          pair<bool, vector<pair<int, int>>> check_for_l2_evict = l2.insert_to_l2(l2_index, l2_tag, config.counter, is_write, -1, memory_refs, dc_index, dc_tag);
          if (check_for_l2_evict.first) { // go through the invalidated dc blocks due to l2 eviction and invalidate them

            vector<pair<int, int>> dc_index_and_tags_to_invalidate = check_for_l2_evict.second;
            for (int i = 0; i < dc_index_and_tags_to_invalidate.size(); i++) {
              if (dc_index_and_tags_to_invalidate[i].first == -1) {
                continue;
              }
              dc.invalidate_bc_l2_eviction(dc_index_and_tags_to_invalidate[i].first, dc_index_and_tags_to_invalidate[i].second, l2_hits, memory_refs);
            }

            config.counter += 1;
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

        page_off_bin = bin_string.substr(64 - config.pt_offset_bit);
        page_off = stoi(page_off_bin, 0, 2);
        cout << setw(4) << hex << setfill(' ') << page_off;

        pfn = pt.check_page_table(virt_page_num, config.counter, is_write);
        if (pfn == -1) {
          pfn = pt.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, is_write, page_off, config.pt_offset_bit, config.l2_enabled, dc, l2, disk_refs, memory_refs, l2_hits, pt_refs, config.dtlb_enabled, dtlb).second;
          pt_faults += 1;
          pt_refs += 1;
          disk_refs += 1;
          pt_res = "miss";
        } else {
          pt_refs += 1;
          pt_hits += 1;
          pt_res = "hit ";
        }
        cout << setw(21) << setfill(' ') << pt_res;

        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = pt.translate_to_phys_address(pfn, page_off, config.pt_offset_bit);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        // cout << " ->" << config.pt_offset_bit << "<- ";
        phys_page_num_bin = substr_safe(p_bin_string, 0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = p_bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;
        dc_index_bin = p_bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index << " ";
        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << "hit ";
          dc_hits += 1;
          config.counter += 1; // if so then its a dc hit
          if (is_write) {
            l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter);
          }
          if (!is_write) {
            cout << " \n";
            continue;
            // memory_refs += 1;
          }

        } else {
          cout << setfill(' ') << setw(4) << "miss";
          config.counter += 1;
          dc_misses += 1;
          // memory_refs;
          if (!is_write) {
            pair<bool, string> was_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          }
          if (is_write) {
            l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter);
          }
          // memory_refs += 1; // cant be dirty so a miss means that we only grab the block from memory
          //  dont have to write back a dirty block
        }
        // start l2 calculations
        l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
        l2_tag = stoi(l2_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";
        l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
        if (config.l2_index_bits == 0) {
          l2_index = 0;
        } else {
          l2_index = stoi(l2_index_bin, 0, 2);
        }
        cout << setw(3) << setfill(' ') << hex << l2_index;
        // check if the block was in l2
        if (l2.check_l2(l2_index, l2_tag, config.counter, is_write, -1, false)) {
          l2_hits += 1; // was found so l2 hit
          cout << " hit \n";
          config.counter += 1;
        } else {
          // was not found, so l2 miss and need to reference memory to get it.
          cout << " miss\n";
          memory_refs += 1;
          l2_misses += 1;
          int temp_mem_refs = memory_refs;
          // so basically we need this temp counter as the insert and invalidate functions will increment them but
          // we dont want to double count ie a dirty dc is invalidated but the l2 was already dirty so it should just be 1
          // memory access we can incremement the temp in the dc eviction function and the normal in the l2 insert
          // we then take the larger of the 2
          pair<bool, vector<pair<int, int>>> check_for_l2_evict = l2.insert_to_l2(l2_index, l2_tag, config.counter, is_write, -1, memory_refs, dc_index, dc_tag);
          if (check_for_l2_evict.first) { // go through the invalidated dc blocks due to l2 eviction and invalidate them

            vector<pair<int, int>> dc_index_and_tags_to_invalidate = check_for_l2_evict.second;
            for (int i = 0; i < dc_index_and_tags_to_invalidate.size(); i++) {
              if (dc_index_and_tags_to_invalidate[i].first == -1) {
                continue;
              }
              dc.invalidate_bc_l2_eviction(dc_index_and_tags_to_invalidate[i].first, dc_index_and_tags_to_invalidate[i].second, l2_hits, memory_refs);
            }

            config.counter += 1;
          }
        }
      } else if (config.virt_addr && config.dtlb_enabled) {
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

        dtlb_index_and_tag = dtlb_index_tag_getter(virt_page_num, config.dtlb_set_count);
        dtlb_index = dtlb_index_and_tag.first;
        dtlb_tag = dtlb_index_and_tag.second;
        cout << setw(7) << setfill(' ') << hex << dtlb_tag;
        cout << setw(4) << setfill(' ') << hex << dtlb_index;

        dtlb_search_result = dtlb.check_dtlb(dtlb_index, dtlb_tag, config.counter);
        if (dtlb_search_result.first) {
          cout << setw(4) << setfill(' ') << " hit ";
          cout << setw(5) << setfill(' ') << " ";
          dtlb_hits += 1;
          pfn = dtlb_search_result.second;
          config.counter += 1;
          pt.check_page_table(virt_page_num, config.counter, is_write);
        } else {
          cout << setw(4) << setfill(' ') << " miss";
          dtlb_misses += 1;
          pfn = pt.check_page_table(virt_page_num, config.counter, is_write);
          if (pfn == -1) {
            pfn = pt.insert_page(virt_page_num, config.virtual_page_count, config.physical_page_count, config.counter, is_write, page_off, config.pt_offset_bit, config.l2_enabled, dc, l2, disk_refs, memory_refs, l2_hits, pt_refs, config.dtlb_enabled, dtlb).second;
            pt_faults += 1;
            pt_refs += 1;
            disk_refs += 1;
            pt_res = "miss";
          } else {
            pt_refs += 1;
            pt_hits += 1;
            pt_res = "hit ";
          }
          dtlb.insert_to_dtlb(dtlb_index, dtlb_tag, config.counter, pfn);
          cout << setw(5) << setfill(' ') << pt_res;
        }

        // PHYSICAL ADDRESS TRANSFORMATION
        int physical_address = pt.translate_to_phys_address(pfn, page_off, config.pt_offset_bit);
        p = bitset<64>(physical_address);
        p_bin_string = p.to_string();

        // PHYSICAL PAGE NUMBER FOR VIRTUAL ADDRESSES
        // cout << " ->" << config.pt_offset_bit << "<- ";
        phys_page_num_bin = substr_safe(p_bin_string, 0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        cout << setw(5) << setfill(' ') << hex << phys_page_num;

        // DC TAG FOR VIRTUAL ADDRESSES
        dc_tag_bin = p_bin_string.substr(0, (64 - (config.dc_index_bits + config.dc_offset_bits)));
        dc_tag = stoi(dc_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << dc_tag;
        dc_index_bin = p_bin_string.substr((64 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
        if (config.dc_index_bits == 0) {
          dc_index = 0;
        } else {
          dc_index = stoi(dc_index_bin, 0, 2);
        }
        cout << " " << setw(3) << setfill(' ') << hex << dc_index << " ";
        if (dc.check_cache(dc_index, dc_tag, config.counter, is_write, pfn, false)) {
          cout << setfill(' ') << setw(4) << "hit ";
          dc_hits += 1;
          config.counter += 1; // if so then its a dc hit
          if (is_write) {
            l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter);
          }
          if (!is_write) {
            cout << " \n";
            continue;
            // memory_refs += 1;
          }

        } else {
          cout << setfill(' ') << setw(4) << "miss";
          config.counter += 1;
          dc_misses += 1;
          // memory_refs;
          if (!is_write) {
            pair<bool, string> was_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, pfn, p_bin_string);
          }
          if (is_write) {
            l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
            l2_tag = stoi(l2_tag_bin, 0, 2);
            l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
            if (config.l2_index_bits == 0) {
              l2_index = 0;
            } else {
              l2_index = stoi(l2_index_bin, 0, 2);
            }
            l2.update_dirty_bit(l2_index, l2_tag, config.counter);
          }
          // memory_refs += 1; // cant be dirty so a miss means that we only grab the block from memory
          //  dont have to write back a dirty block
        }
        // start l2 calculations
        l2_tag_bin = p_bin_string.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
        l2_tag = stoi(l2_tag_bin, 0, 2);
        cout << " " << setw(6) << setfill(' ') << hex << l2_tag << " ";
        l2_index_bin = p_bin_string.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
        if (config.l2_index_bits == 0) {
          l2_index = 0;
        } else {
          l2_index = stoi(l2_index_bin, 0, 2);
        }
        cout << setw(3) << setfill(' ') << hex << l2_index;
        // check if the block was in l2
        if (l2.check_l2(l2_index, l2_tag, config.counter, is_write, -1, false)) {
          l2_hits += 1; // was found so l2 hit
          cout << " hit \n";
          config.counter += 1;
        } else {
          // was not found, so l2 miss and need to reference memory to get it.
          cout << " miss\n";
          memory_refs += 1;
          l2_misses += 1;
          int temp_mem_refs = memory_refs;
          // so basically we need this temp counter as the insert and invalidate functions will increment them but
          // we dont want to double count ie a dirty dc is invalidated but the l2 was already dirty so it should just be 1
          // memory access we can incremement the temp in the dc eviction function and the normal in the l2 insert
          // we then take the larger of the 2
          pair<bool, vector<pair<int, int>>> check_for_l2_evict = l2.insert_to_l2(l2_index, l2_tag, config.counter, is_write, -1, memory_refs, dc_index, dc_tag);
          if (check_for_l2_evict.first) { // go through the invalidated dc blocks due to l2 eviction and invalidate them

            vector<pair<int, int>> dc_index_and_tags_to_invalidate = check_for_l2_evict.second;
            for (int i = 0; i < dc_index_and_tags_to_invalidate.size(); i++) {
              if (dc_index_and_tags_to_invalidate[i].first == -1) {
                continue;
              }
              dc.invalidate_bc_l2_eviction(dc_index_and_tags_to_invalidate[i].first, dc_index_and_tags_to_invalidate[i].second, l2_hits, memory_refs);
            }

            config.counter += 1;
          }
        }
      }
    }
    config.counter += 1;
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

// with small page amount of physical pages stoi would break and this was the solution
//  and error checking
string substr_safe(const string &s, int pos, int len) {
  int string_length = s.length();
  if (pos < 0 || pos > string_length) {
    cerr << "substr_safe: requested pos " << pos << " is out of range for string of size " << string_length << "\n";
    exit(-1);
  }
  if (len < 0) {
    cerr << "substr_safe: requested len " << len << " is negative\n";
    exit(-1);
  }
  if (pos + len > string_length) {
    // clamp length to available characters
    len = string_length - pos;
  }
  return s.substr(pos, len);
}

pair<int, int> dtlb_index_tag_getter(int vpn, int tlb_set_count) {
  // i orignally calculated this like i did the other indexs and tags
  // but i kept getting errors on it so bit shift time.
  int tlb_index_bits = 0;
  int sets = tlb_set_count;
  while (sets > 1) {
    sets >>= 1;
    tlb_index_bits++;
    // increment the amount of bits and right shift
    // the amount of sets by 1. this is equivelant to power of 2
    // 8 = 1000 -> 0100 = 4, tlb_index_bits=1
    // 0100-> 0010 = 2, tlb_index_bits=2
    // 0010 ->0001 = 1, tlb_index_bits=3
  }
  // shift 1 to the left, tlb_index_bit times and subtrace 1
  // ex: 1 << 3 = 8
  // 8 - 1 = 7 or 0111
  // this will make a mask for the index
  int index_mask = (1ULL << tlb_index_bits) - 1;
  int index = vpn & index_mask;
  // tag is the remaining parts of the vpn, so right shift it
  // tlb_index_bits times
  int tag = vpn >> tlb_index_bits;

  return {index, tag};
}