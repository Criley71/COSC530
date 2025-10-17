#include "config.h"
using namespace std;

Config::Config(){
  ifstream fin;
  fin.open("config.txt");
  if(!fin.good()){
    cout << "Error Could not find config.txt, exiting\n";
    exit(1);
  }
  int colon_index;
  string val;
  string line;
  while(getline(fin, line)){
    if(!line.empty()){
      if(line.find("eff addr") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        eff_add_buffer = stoi(val);
        val = "";
      }else if(line.find("fp adds") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        fp_adds_buffer = stoi(val);
        val = "";
      }else if(line.find("fp muls") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        fp_muls_buffer = stoi(val);
        val = "";
      }else if(line.find("ints") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        ints_buffer = stoi(val);
        val = "";
      }else if(line.find("reorder") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        reorder_buffer = stoi(val);
        val = "";
      }else if(line.find("fp_add") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        fp_add_latency = stoi(val);
        val = "";
      }else if(line.find("fp_sub") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        fp_sub_latency = stoi(val);
        val = "";
      }else if(line.find("fp_mul") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        fp_mul_latency= stoi(val);
        val = "";
      }else if(line.find("fp_div") == 0){
        colon_index = line.find(":") + 2;
        val = line.substr(colon_index, line.size());
        fp_div_latency = stoi(val);
        val = "";
      }
    }
  }


}