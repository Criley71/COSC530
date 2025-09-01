#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <typeinfo>
#include <bitset>
#include <sstream>
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
      } else if (line.find("Virtual addresses") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        if(val == "y"){
          config.virt_addr = true;
        }else if (val == "n"){
          config.virt_addr = false;
        }
        val = "";
      } else if (line.find("TLB") == 0){
        colon_index = line.find(":") + 2;
        if(val == "y"){
          config.dtlb_enabled = true;
        }else if (val == "n") {
          config.dtlb_enabled = false;
        }
        val = "";
      } else if (line.find("L2 cache") == 0){
        colon_index = line.find(":") + 2;
        if(val == "y"){
          config.l2_enabled = true;
        }else if (val == "n"){
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
  if(config.virt_addr){
    cout << "The addresses read in are virtual addresses.\n";
  } else{
    cout << "The addresses read in are physical addresses.\n";
  }
  if (!config.dtlb_enabled){
    cout << "TLB is disabled in this configuration.\n";
  }
  if(!config.l2_enabled){
    cout << "L2 cache is disabled in this configuration.\n\n";
  }
  cout << "Physical Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n";
  cout << "Address  Page # Off  Tag    Ind Res. Res. Pg # DC Tag Ind Res. L2 Tag Ind Res.\n";
  cout << "-------- ------ ---- ------ --- ---- ---- ---- ------ --- ---- ------ --- ----\n";
}

void read_data_file(Config config){
  int address;
  int virt_page_num;
  int page_off;
  int tlb_tag;
  int tlb_index;
  string tlb_res;
  string pt_res;
  int phys_page_num;
  int dc_tag;
  int dc_index;
  int dc_res;
  int l2_tag;
  int l2_ind;
  bool is_read; //false will be on write, true on read
  string l2_res;

  ifstream fin("trace.dat");
  string line;
  string hex_val;
  stringstream ss;
  bitset<32> b;
  
  while(getline(fin, line)){
    if(line.find("R") == 0){
      hex_val = line.substr(2, line.size());
      cout << hex_val << "\n";
      ss << hex << hex_val;
      unsigned n;
      ss >> n;
      ss.clear();
      
      b = bitset<32>(n);
      b.to_string();
      cout << b << "\n";
    }
  }
  
}