#include "config.h"
using namespace std;

Config::Config() {
  ifstream fin("trace.config");
  counter = 0;
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
            dtlb_set_count = stoi(val);

            val = "";
            if (!check_pwr_2(dtlb_set_count)) {
              cout << "DTLB set count is not a power of 2. Exiting\n";
              exit(-1);
            }
            dltb_index_bits = log2(dtlb_set_count);
          } else if (line.find("Set size") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            dtlb_set_size = stoi(val);
            val = "";
          }
        }
      } else if (line.find("Page Table configuration") == 0) {
        for (int i = 0; i < 3; i++) {
          getline(fin, line);
          if (line.find("Number of virtual pages") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            virtual_page_count = stoi(val);

            val = "";
            if (!check_pwr_2(virtual_page_count)) {
              cout << "Number of virtual pages is not a power 2. Exiting\n";
              exit(-1);
            }
            pt_index_bits = log2(virtual_page_count);

          } else if (line.find("Number of physical pages") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            physical_page_count = stoi(val);

            val = "";
            if (!check_pwr_2(physical_page_count)) {
              cout << "Number of Physical pages is not a power 2. Exiting\n";
              exit(-1);
            }
          } else if (line.find("Page size") == 0) {
            colon_index = line.find(':') + 2;
            val = line.substr(colon_index, line.size());
            page_size = stoi(val);

            val = "";
            if (!check_pwr_2(page_size)) {
              cout << "Page size is not a power 2. Exiting\n";
              exit(-1);
            }
            pt_offset_bit = log2(page_size);
          }
        }
      } else if (line.find("Data Cache configuration") == 0) {
        for (int i = 0; i < 4; i++) {
          getline(fin, line);
          if (line.find("Number of sets") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            dc_set_count = stoi(val);
            val = "";
            if (!check_pwr_2(dc_set_count)) {
              cout << "Number of DC sets is not a power of 2. Exiting\n";
              exit(-1);
            }
            dc_index_bits = log2(dc_set_count);
          } else if (line.find("Set size") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            dc_set_size = stoi(val);
            val = "";
            /*
            if (!check_pwr_2(dc_set_size)) {
              cout << "Number of DC entries is not a power of 2. Exiting\n";
              exit(-1);
            }
              */
          } else if (line.find("Line size") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            dc_line_size = stoi(val);
            val = "";
            if (!check_pwr_2(dc_line_size)) {
              cout << "Number of DC line size is not a power of 2. Exiting\n";
              exit(-1);
            }
            dc_offset_bits = log2(dc_line_size);
          } else if (line.find("Write through/no write allocate") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            if (val == "n") {
              dc_write_thru_no_allo = false;
            } else {
              dc_write_thru_no_allo = true;
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
            l2_set_count = stoi(val);
            val = "";
            if (!check_pwr_2(l2_set_count)) {
              cout << "Number of sets in the L2 cache are not a power of 2. Exiting\n";
              exit(-1);
            }
            l2_index_bits = log2(l2_set_count);
          } else if (line.find("Set size") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            l2_set_size = stoi(val);
            val = "";
          } else if (line.find("Line size") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            l2_line_size = stoi(val);
            val = "";
            if (!check_pwr_2(l2_line_size)) {
              cout << "L2 line size is not a power of 2. Exiting\n";
              exit(-1);
            }
            if (l2_line_size < dc_line_size) {
              cout << "L2 line size is smaller dc line size. Exiting\n";
              exit(-1);
            }
            l2_offset_bits = log2(l2_line_size);
          } else if (line.find("Write through/no write allocate") == 0) {
            colon_index = line.find(":") + 2;
            val = line.substr(colon_index, line.size());
            if (val == "n") {
              l2_write_thru_no_allo = false;
            } else if (val == "y") {
              l2_write_thru_no_allo = true;
            }
            val = "";
          }
        }
      } else if (line.find("Virtual addresses") == 0) {
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        if (val == "y") {
          virt_addr = true;
        } else if (val == "n") {
          virt_addr = false;
        }
        val = "";
      } else if (line.find("TLB") == 0) {
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        if (val == "y") {
          dtlb_enabled = true;
        } else if (val == "n") {
          dtlb_enabled = false;
        }
        val = "";
      } else if (line.find("L2 cache") == 0) {
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        if (val == "y") {
          l2_enabled = true;
        } else if (val == "n") {
          l2_enabled = false;
        }
        val = "";
      }
    }
  }
  l2_to_dc_ratio = l2_line_size / dc_line_size;
}

bool Config::check_pwr_2(int val){// 8 = 1000, 7 = 0111 ---> 1000 & 0111 = 0000
    return (0 == ((val - 1) & val));
}

void Config::print_config_after_read() {
  cout << "Data TLB contains " << dtlb_set_count << " sets.\n";
  cout << "Each set contains " << dtlb_set_size << " entries.\n";
  cout << "Number of bits used for the index is " << dltb_index_bits << ".\n\n";
  cout << "Number of virtual pages is " << virtual_page_count << ".\n";
  cout << "Number of physical pages is " << physical_page_count << ".\n";
  cout << "Each page contains " << page_size << " bytes.\n";
  cout << "Number of bits used for the page table index is " << pt_index_bits << ".\n";
  cout << "Number of bits used for the page offset is " << pt_offset_bit << ".\n\n";
  cout << "D-cache contains " << dc_set_count << " sets.\n";
  cout << "Each set contains " << dc_set_size << " entries.\n";
  cout << "Each line is " << dc_line_size << " bytes.\n";
  if (dc_write_thru_no_allo) {
    cout << "The cache uses a no write-allocate and write-through policy.\n";
  } else {
    cout << "The cache uses a write-allocate and write-back policy.\n";
  }
  cout << "Number of bits used for the index is " << dc_index_bits << ".\n";
  cout << "Number of bits used for the offset is " << dc_offset_bits << ".\n\n";
  cout << "L2-cache contains " << l2_set_count << " sets.\n";
  cout << "Each set contains " << l2_set_size << " entries.\n";
  cout << "Each line is " << l2_line_size << " bytes.\n";
  if (l2_write_thru_no_allo) {
    cout << "The cache uses a no write-allocate and write-through policy.\n";
  } else {
    cout << "The cache uses a write-allocate and write-back policy.\n";
  }
  cout << "Number of bits used for the index is " << l2_index_bits << ".\n";
  cout << "Number of bits used for the offset is " << l2_offset_bits << ".\n\n";
  if (virt_addr) {
    cout << "The addresses read in are virtual addresses.\n";
  } else {
    cout << "The addresses read in are physical addresses.\n";
  }
  if (!dtlb_enabled) {
    cout << "TLB is disabled in this configuration.\n";
  }
  if (!l2_enabled) {
    cout << "L2 cache is disabled in this configuration.\n";
  }

  cout << "\n";
  if (virt_addr) {
    cout << "Virtual  Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n";
  } else {
    cout << "Physical Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n";
  }
  cout << "Address  Page # Off  Tag    Ind Res. Res. Pg # DC Tag Ind Res. L2 Tag Ind Res.\n";
  cout << "-------- ------ ---- ------ --- ---- ---- ---- ------ --- ---- ------ --- ----\n";
}