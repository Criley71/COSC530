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

    mt19937 gen(rd());

    uniform_int_distribution<> reorder_dist(1, 10);
    int reorder = reorder_dist(gen);

    // ----------- Correct random partitioning into 4 fields -----------
    // We want 4 numbers >= 1 that sum to exactly 10.
    int total = 10;
    int fields = 4;

    // Start with all 1s
    vector<int> values(fields, 1);

    int remaining = total - fields; // remaining = 10 - 4 = 6

    // Generate 3 cut points between 0 and remaining (inclusive)
    vector<int> cuts;
    uniform_int_distribution<> cut_dist(0, remaining);

    for (int i = 0; i < fields - 1; i++)
        cuts.push_back(cut_dist(gen));

    sort(cuts.begin(), cuts.end());

    // Convert cuts into increments
    vector<int> increments(fields, 0);
    increments[0] = cuts[0];
    for (int i = 1; i < fields - 1; i++)
        increments[i] = cuts[i] - cuts[i - 1];
    increments[fields - 1] = remaining - cuts.back();

    // Add increments to the base 1s
    for (int i = 0; i < fields; i++)
        values[i] += increments[i];

    int eff_addr = values[0];
    int fp_adds  = values[1];
    int fp_muls  = values[2];
    int ints     = values[3];

    // ----------- Randomize latencies -----------
    uniform_int_distribution<> fp_add_dist(1, 2);
    uniform_int_distribution<> fp_sub_dist(1, 2);
    uniform_int_distribution<> fp_mul_dist(1, 5);
    uniform_int_distribution<> fp_div_dist(1, 10);

    int fp_add = fp_add_dist(gen);
    int fp_sub = fp_sub_dist(gen);
    int fp_mul = fp_mul_dist(gen);
    int fp_div = fp_div_dist(gen);

    // ----------- Output Results -----------
    ofstream ofile("config.txt");
    if(!ofile.good()){
        return 1;
    }
    ofile << "buffers\n\n";
    ofile << "eff addr: " << eff_addr << "\n";
    ofile << "fp adds:  " << fp_adds << "\n";
    ofile << "fp muls:  " << fp_muls << "\n";
    ofile << "ints:     " << ints << "\n";
    ofile << "reorder:  " << reorder << "\n\n";

    ofile << "latencies\n\n";
    ofile << "fp_add: " << fp_add << "\n";
    ofile << "fp_sub: " << fp_sub << "\n";
    ofile << "fp_mul: " << fp_mul << "\n";
    ofile << "fp_div: " << fp_div << "\n";

    return 0;

    return 0;
}