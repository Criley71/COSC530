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

  bool virt_addr;
};

Config init(); // function to read in config and set variables
bool check_pwr_2(int val);
void print_config_after_read(Config config);
void read_data_file(Config config);
string binary_to_hex(string bin_val);

int main(int argc, char *argv[]) {
  Config config = init();
  print_config_after_read(config);
  read_data_file(config);
}

Config init() {
  Config config;
  ifstream fin("trace.config");

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
            // cout << "dltb set count: " << config.dtlb_set_count << "\n";
            //  //cout << typeid(config.dtlb_set_count).name() << "\n";
            val = "";
            if (!check_pwr_2(config.dtlb_set_count)) {
              cout << "DTLB set count is not a power of 2. Exiting\n";
              exit(-1);
            }
            config.dltb_index_bits = log2(config.dtlb_set_count);
            // cout << "Number of bits DLTB uses for index: " << config.dltb_index_bits << "\n";
          } else if (line.find("Set size") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            config.dtlb_set_size = stoi(val);
            // cout << "dltb set size: " << config.dtlb_set_size << "\n";
            //  //cout << typeid(config.dtlb_set_size).name() << "\n";
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
            // cout << "number of virtual pages: " << config.virtual_page_count << "\n";
            //  //cout << typeid(config.virtual_page_count).name() << "\n";
            val = "";
            if (!check_pwr_2(config.virtual_page_count)) {
              cout << "Number of virtual pages is not a power 2. Exiting\n";
              exit(-1);
            }
            config.pt_index_bits = log2(config.virtual_page_count);
            // cout << "Number of bits used for the page table index is " << config.pt_index_bits << "\n";
          } else if (line.find("Number of physical pages") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            config.physical_page_count = stoi(val);
            // cout << "number of phys pages: " << config.physical_page_count << "\n";
            //  //cout << typeid(config.physical_page_count).name() << "\n";
            val = "";
            if (!check_pwr_2(config.physical_page_count)) {
              cout << "Number of Physical pages is not a power 2. Exiting\n";
              exit(-1);
            }
          } else if (line.find("Page size") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            config.page_size = stoi(val);
            // cout << "page size: " << config.page_size << "\n";
            //  //cout << typeid(config.page_size).name() << "\n";
            val = "";
            if (!check_pwr_2(config.page_size)) {
              cout << "Page size is not a power 2. Exiting\n";
              exit(-1);
            }
            config.pt_offset_bit = log2(config.page_size);
            // cout << "Number of bits used for the page offset is " << "\n";
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
        if (val == "y") {
          config.dtlb_enabled = true;
        } else if (val == "n") {
          config.dtlb_enabled = false;
        }
        val = "";
      } else if (line.find("L2 cache") == 0) {
        colon_index = line.find(":") + 2;
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
    cout << "The cache uses a no-write-allocate and write through policy.\n";
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
    cout << "L2 cache is disabled in this configuration.\n\n";
  }
  cout << "Physical Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n";
  cout << "Address  Page # Off  Tag    Ind Res. Res. Pg # DC Tag Ind Res. L2 Tag Ind Res.\n";
  cout << "-------- ------ ---- ------ --- ---- ---- ---- ------ --- ---- ------ --- ----\n";
}

void read_data_file(Config config) {
  int address;
  int virt_page_num;
  int page_off;
  string page_off_bin;
  string page_off_hex;
  int tlb_tag;
  int tlb_index;
  string tlb_res;
  string pt_res;
  int phys_page_num;
  string phys_page_num_bin;
  string phys_page_num_hex;
  int dc_tag;
  string dc_tag_bin;
  string dc_tag_hex;
  int dc_index;
  string dc_index_bin;
  string dc_index_hex;
  int dc_res;
  int l2_tag;
  int l2_ind;
  bool is_read; // false will be on write, true on read
  string l2_res;

  ifstream fin("trace2.dat");
  string line;
  string hex_val;
  string bin_string;
  stringstream ss;
  bitset<32> b;
  DC DATA_CACHE = DC(config.dc_set_count, config.dc_set_size, config.dc_line_size, false, config.dc_index_bits, config.dc_offset_bits);
  
  while (getline(fin, line)) {
    if (config.virt_addr == false) {
      hex_val = line.substr(2, line.size());
      // cout << hex_val << ": ";
      ss << hex << hex_val;
      unsigned n;

      ss >> n;
      // cout << n;
      // if (n >= config.physical_page_count) {
      //  cout << "hierarchy: physical address " << hex << n << " is too large.\n";
      //  exit(-1);
      // }

      b = bitset<32>(n);
      bin_string = b.to_string();
      // cout << bin_string << "\n";
      phys_page_num_bin = bin_string.substr(0, (32 - config.pt_offset_bit));

      phys_page_num_hex = binary_to_hex(phys_page_num_bin);
      phys_page_num = stoi(phys_page_num_hex, 0, 16);

      if (phys_page_num >= config.physical_page_count) {
        cout << "hierarchy: physical address " << hex << n << " is too large.\n";
        exit(-1);
      }
      cout << hex << setw(8) << setfill('0') << n;
      ss.clear();

      cout << " " << setw(7) << setfill(' '); // no v address if false so just print empty space
      page_off_bin = bin_string.substr(32 - (config.pt_offset_bit));
      // cout << page_off_bin.length();
      page_off_hex = binary_to_hex(page_off_bin);
      cout << " " << setw(4) << page_off_hex;
      page_off = stoi(page_off_hex, 0, 16);

      cout << " " << setw(21);
      cout << " " << setw(4) << phys_page_num;

      dc_tag_bin = bin_string.substr(0, (32 - (config.dc_index_bits + config.dc_offset_bits)));
      dc_tag_hex = binary_to_hex(dc_tag_bin);
      dc_tag = stoi(dc_tag_hex, 0, 16);
      cout << " " << setw(6) << dc_tag;

      dc_index_bin = bin_string.substr((32 - (config.dc_index_bits + config.dc_offset_bits)), config.dc_index_bits);
      dc_index_hex = binary_to_hex(dc_index_bin);
      dc_index = stoi(dc_index_bin, 0, 16);
      cout << " " << setw(3) << dc_index;
      if(!DATA_CACHE.check_cache(dc_index, dc_tag)){
        cout << " miss \n";
      }else{
        cout << " hit  \n";
      }
    }
  }
}

// used https://www.geeksforgeeks.org/dsa/convert-binary-number-hexadecimal-number/ for ref
string binary_to_hex(string bin_val) {
  int padding = bin_val.length() % 4;

  if (padding != 0) {
    for (int i = 0; i < padding; i++) {
      bin_val = '0' + bin_val;
    }
  }
  // some vs-code bs so this map doesnt format to be on one line
  // clang-format off
  unordered_map<string, char> hex_map = {
      {"0000", '0'}, {"0001", '1'}, {"0010", '2'}, {"0011", '3'}, 
      {"0100", '4'}, {"0101", '5'}, {"0110", '6'}, {"0111", '7'}, 
      {"1000", '8'}, {"1001", '9'}, {"1010", 'a'}, {"1011", 'b'}, 
      {"1100", 'c'}, {"1101", 'd'}, {"1110", 'e'}, {"1111", 'f'}};
  // clang-format on
  string hex_val;
  for (size_t i = 0; i < bin_val.length(); i += 4) {
    string four_bits = bin_val.substr(i, 4);
    hex_val += hex_map[four_bits];
  }
  size_t zero_check = hex_val.find_first_not_of('0');
  if (zero_check == string::npos) return "0";
  return hex_val;
}