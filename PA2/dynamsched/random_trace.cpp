#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm> 
#include <random>  
  #include <chrono>  
using namespace std;
int main() {
    string input_filename = "trace2.dat";
    string output_filename = "random_trace.dat";

    // 1. Read all lines from the input file into a vector of strings
    ifstream input_file(input_filename);
    if (!input_file.is_open()) {
        cerr << "Error opening input file: " << input_filename << endl;
        return 1;
    }

    vector<string> lines;
    string line;
    while (getline(input_file, line)) {
        lines.push_back(line);
    }
    input_file.close();

    random_device rd;
    mt19937 g(rd()); 


    shuffle(lines.begin(), lines.end(), g);

  
    ofstream output_file(output_filename);
    if (!output_file.is_open()) {
        cerr << "Error opening output file: " << output_filename << endl;
        return 1;
    }

    for (const auto& shuffled_line : lines) {
        output_file << shuffled_line << endl;
    }
    output_file.close();

    cout << "Lines successfully reordered and written to " << output_filename << endl;

    return 0;
}