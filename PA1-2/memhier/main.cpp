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

void read_data_no_virtual(Config config); // reads in the line and will call the appropraite set of functions based on the config
void read_data_virtual(Config config);

void simulate_withOUT_l2_enable(Config config);
void simulate_with_l2_enabled(Config config);

void physical_write_back_dc_no_l2(Config config, string address);
void physical_write_back_dc_with_l2(Config config, string address);
void print_stats(double dtlb_hits, double dtlb_misses, double pt_hits, double pt_faults, double dc_hits, double dc_misses, double l2_hits, double l2_misses, double total_reads, double total_writes, double mem_refs, double pt_refs, double disk_refs);

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

  string bin_string;
  string phys_page_num_bin;
  int phys_page_num;
  string page_off_bin;
  int page_off;
  string dc_tag_bin;
  int dc_tag;
  string dc_index_bin;
  int dc_index;
  DC dc(config.dc_set_count, config.dc_set_size, config.dc_line_size, config.dc_write_thru_no_allo, config.dc_index_bits, config.dc_offset_bits);
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
    config.counter += 1;
    if (!config.dc_write_thru_no_allo) {
      if (!config.virt_addr) {
        phys_page_num_bin = bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        if (phys_page_num >= config.physical_page_count) {
          cout << "hierarchy: physical address " << hex << temp << " is too large.\n";
          exit(-1);
        }
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
          cout << setfill(' ') << setw(4) << "hit  \n";
          dc_hits += 1;
          config.counter += 1;
        } else {
          pair<bool, string> was_replace = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, -1, bin_string);
          if (was_replace.first) {
            memory_refs += 1;
          }
          dc_misses += 1;
          memory_refs += 1;
          cout << setfill(' ') << setw(4) << "miss \n";
          config.counter += 1;
        }
      }
      config.counter += 1;
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

  DC dc(config.dc_set_count, config.dc_set_size, config.dc_line_size, config.dc_write_thru_no_allo, config.dc_index_bits, config.dc_offset_bits);
  L2 l2(config.l2_set_count, config.l2_set_size, config.l2_line_size, config.l2_write_thru_no_allo, config.l2_index_bits, config.l2_offset_bits, config.l2_to_dc_ratio);
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
      if (!config.virt_addr) { //No virtual address enabled
        //begin calculating basic information and make sure address is valid
        phys_page_num_bin = bin_string.substr(0, (64 - config.pt_offset_bit));
        phys_page_num = stoi(phys_page_num_bin, 0, 2);
        if (phys_page_num >= config.physical_page_count) {
          cout << "hierarchy: physical address " << hex << temp << " is too large.\n";
          exit(-1);
        }
        //print out addres, page offset and page num
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
        //start calculating dc stuff
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
          cout << setfill(' ') << setw(4) << "hit  \n";
          dc_hits += 1;
          continue;
          //address was found in dc
        } else {
          //address wasnt found. need to check if a block was evicted to then see if it was dirty and write that to l2
          pair<bool, string> was_dc_replaced_and_was_dirty = dc.insert_to_cache(dc_index, dc_tag, config.counter, is_write, -1, bin_string);
          dc_misses += 1;

          cout << setfill(' ') << setw(4) << "miss";
          bool dirty_replaced = was_dc_replaced_and_was_dirty.first;
          if(dirty_replaced){
            l2_hits+=1;
            string old_address = was_dc_replaced_and_was_dirty.second;
            l2_tag_bin = old_address.substr(0, (64 - (config.l2_index_bits + config.l2_offset_bits)));
              l2_tag = stoi(l2_tag_bin, 0, 2);
              l2_index_bin = old_address.substr((64 - (config.l2_index_bits + config.l2_offset_bits)), config.l2_index_bits);
              if (config.l2_index_bits == 0) {
                l2_index = 0;
              } else {
                l2_index = stoi(l2_index_bin, 0, 2);
              }
              l2.update_dirty_bit(l2_index, l2_tag, config.counter);
              config.counter += 1;
          }
        }
        //start l2 calculations
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
        
        if (l2.check_l2(l2_index, l2_tag, config.counter, is_write, -1, false)) {
          l2_hits+=1;
          cout << " hit \n";
          config.counter+=1;
        }else{
          cout << " miss\n";
          memory_refs+=1;
          l2_misses += 1;
          pair<bool, vector<pair<int, int>>> check_for_l2_evict = l2.insert_to_l2(l2_index, l2_tag, config.counter, is_write, -1, memory_refs, dc_index, dc_tag);
          if(check_for_l2_evict.first){
            vector<pair<int, int>> dc_index_and_tags_to_invalidate = check_for_l2_evict.second;
            for(int i = 0; i < dc_index_and_tags_to_invalidate.size(); i++){
              if(dc_index_and_tags_to_invalidate[i].first == -1){
                continue;
              }
              dc.invalidate_bc_l2_eviction(dc_index_and_tags_to_invalidate[i].first, dc_index_and_tags_to_invalidate[i].second, l2_hits, memory_refs);
            }
            config.counter+=1;
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
