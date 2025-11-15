#include <iostream>
#include <vector>
using namespace std;

/*

CANVAS ONLY ALLOWS 1 FILE SO I CANT SUPPLY A MAKE FILE, HERE IS COMPILE AND RUN COMMANDS
compile:
g++ -o wa5 wa5.cpp
run:
./wa4

*/

void question_one(int b1_pred_init, int b2_pred_init, int b3_pred_init);
void question_two(int b1_pred_init, int b2_pred_init, int b3_pred_init);
void question_three(int b1_pred_init, int b2_pred_init, int b3_pred_init);
void question_four(int b1_pred_init, int b2_pred_init, int b3_pred_init);

int main() {
  cout << "QUESTION 1" << "\n";
  question_one(1, 0, 0);
  cout << "\nQUESTION 2" << "\n";
  question_two(1, 2, 2);
  cout << "\nQUESTION 3" << "\n";
  question_three(0, 0, 0);
  cout << "\nQUESTION 4" << "\n";
  question_four(0, 0, 0);
}
void question_one(int b1_pred_init, int b2_pred_init, int b3_pred_init) {
  int sum = 0;

  int b1_pred = b1_pred_init;
  int b2_pred = b2_pred_init;
  int b3_pred = b3_pred_init;

  int b1_hits = 0;
  int b2_hits = 0;
  int b3_hits = 0;

  int b1_misses = 0;
  int b2_misses = 0;
  int b3_misses = 0;
  if (b3_pred == 0) {
    b3_misses += 1;
    b3_pred = 1;
  } else {
    b3_hits += 1;
  }
  for (int i = 1; i < 101; i++) { // B3, outer loop condition: if (i < 101) continue to inner part of outer

    if (i % 4 == 0) {     // Branch is taken
      if (b1_pred == 0) { // Predicted not taken but was taken
        b1_misses += 1;
        b1_pred = 1; // Update prediction to taken
      } else {       // Predicted taken and was taken
        b1_hits += 1;
      }
    } else {              // Branch is not taken
      if (b1_pred == 1) { // Predicted taken but was not taken
        b1_misses += 1;
        b1_pred = 0; // Update prediction to not taken
      } else {       // Predicted not taken and was not taken
        b1_hits += 1;
        // Don't change prediction when correct
      }
    }
    //  cout << "i: " << i << " b1: " << b1_misses << ", ";
    if ((i % 4) == 0) {   // B1, if ((i%4) != 0) skip the inner loop
      if (b2_pred == 0) { // Predicted not taken but was taken
        b2_misses += 1;
        b2_pred = 1; // Update prediction
      } else {       // Predicted taken and was taken
        b2_hits += 1;
      }
      for (int j = 1; j < 11; j++) { // B2, inner loop condition: if (j < 11) continue inner loop

        sum += i * j;
        if (j < 10) {         // Branch actually taken
          if (b2_pred == 0) { // Predicted not taken but was taken
            b2_misses += 1;
            b2_pred = 1; // Update prediction
          } else {       // Predicted taken and was taken
            b2_hits += 1;
          }
        } else {              // Branch actually not taken (j == 10)
          if (b2_pred == 1) { // Predicted taken but was not taken
            b2_misses += 1;
            b2_pred = 0; // Update prediction
          } else {       // Predicted not taken and was not taken
            b2_hits += 1;
          }
        }
      }
      // cout << i << ":" << b2_misses << ", ";
    }
    sum += i;
    if (b3_pred && i != 100) {
      b3_hits += 1;
    } else {
      b3_misses += 1;
      b3_pred = 1;
    }
  }

  cout << "b1 hits: " << b1_hits << "\nb1 misses: " << b1_misses << "\n";
  cout << "b2 hits: " << b2_hits << "\nb2 misses: " << b2_misses << "\n";
  cout << "b3 hits: " << b3_hits << "\nb3 misses: " << b3_misses << "\n";
}

void question_two(int b1_pred_init, int b2_pred_init, int b3_pred_init) {
  int sum = 0;
  /*
  00 = 0 nt
  01 = 1 nt
  10 = 2 t
  11 = 3 t
  pred << result (1 or 0) && 0b11
   */

  int b1_pred = b1_pred_init; // 01
  int b2_pred = b2_pred_init; // 10
  int b3_pred = b3_pred_init; // 10

  int b1_hits = 0;
  int b2_hits = 0;
  int b3_hits = 0;

  int b1_misses = 0;
  int b2_misses = 0;
  int b3_misses = 0;
  // cout << "b3_pred " << b3_pred << "\n";

  if (b3_pred < 2) {
    b3_misses += 1;
    b3_pred = (b3_pred << 1) | 1;
    b3_pred &= 0b11;
  } else {
    b3_hits += 1;
    b3_pred |= 1;
    b3_pred &= 0b11;
  }
  for (int i = 1; i < 101; i++) { // b3
    if (i % 4 == 0) {             // b1
      if (b1_pred == 0) {
        b1_misses += 1;
        b1_pred = 1;
      } else if (b1_pred == 1) {
        b1_misses += 1;
        b1_pred = 2;
      } else {
        if (b1_pred == 2) {
          b1_hits += 1;
          b1_pred = 3;
        } else {
          b1_hits += 1;
          b1_pred = 3;
        }
      }
    } else {
      if (b1_pred == 0) {
        b1_hits += 1;
        b1_pred = 0;
      } else if (b1_pred == 1) {
        b1_hits += 1;
        b1_pred = 0;
      } else if (b1_pred == 2) {
        b1_misses += 1;
        b1_pred = 1;
      } else if (b1_pred == 3) {
        b1_misses += 1;
        b1_pred = 2;
      }
    }
    if ((i % 4) == 0) { // b1
      if (b2_pred < 2) {
        b2_misses += 1;
        b2_pred += 1;
      } else {
        b2_hits += 1;
        b2_pred = 3;
      }
      for (int j = 1; j < 11; j++) { // b2
        sum += i * j;

        if (b2_pred >= 2 && j != 10) {
          b2_hits += 1;
          b2_pred = 3;
        } else {
          b2_misses += 1;
          if (b2_pred != 0) {
            b2_pred -= 1;
          }
        }
      }
    }
    sum += i;
    // cout << "b3_pred " << b3_pred << "\n";
    if (b3_pred > 1 && i != 100) {
      b3_hits += 1;
      b3_pred = (b3_pred << 1) | 1;
      b3_pred &= 0b11;
    } else {
      b3_misses += 1;
      b3_pred = (b3_pred << 1) | 1;
      b3_pred &= 0b11;
    }
  }

  cout << "b1 hits: " << b1_hits << "\nb1 misses: " << b1_misses << "\n";
  cout << "b2 hits: " << b2_hits << "\nb2 misses: " << b2_misses << "\n";
  cout << "b3 hits: " << b3_hits << "\nb3 misses: " << b3_misses << "\n";
}

void question_three(int b1_pred_init, int b2_pred_init, int b3_pred_init) {
  int sum = 0;

  /*
  00 = 0 nt
  01 = 1 nt
  10 = 2 t
  11 = 3 t
  */
  //(m,n) 2^m entries, n bit predictor. 1 bit global

  int global_pred = 0; // 1 bit global
  vector<int> b1_pred = {0b00, 0b00};
  vector<int> b2_pred = {0b00, 0b00};
  vector<int> b3_pred = {0b00, 0b00};

  int b1_hits = 0;
  int b2_hits = 0;
  int b3_hits = 0;

  int b1_misses = 0;
  int b2_misses = 0;
  int b3_misses = 0;

  if (b3_pred[global_pred] < 2) {
    b3_misses += 1;
    b3_pred[global_pred] += 1;
  } else {
    b3_hits += 1;
    b3_pred[global_pred] = 3;
  }

  for (int i = 1; i < 101; i++) { // b3
    global_pred = 1;              // b3 was taken, set to 1
    if ((i % 4) == 0) {
      if (b1_pred[global_pred] < 2) {
        b1_hits += 1;
        if (b1_pred[global_pred] > 0) {
          b1_pred[global_pred] -= 1;
        }
      } else {
        b1_misses += 1;
        if (b1_pred[global_pred] > 0) {
          b1_pred[global_pred] -= 1;
        }
      }
      global_pred = 0;
    } else {
      if (b1_pred[global_pred] < 2) {
        b1_misses += 1;
        if (b1_pred[global_pred] < 3) {
          b1_pred[global_pred] += 1;
        }
      } else {
        b1_hits += 1;
        if (b1_pred[global_pred] < 3) {
          b1_pred[global_pred] += 1;
        }
      }
      global_pred = 1;
    }
    if ((i % 4) == 0) {
      // b1 taken, set global to 1
      //global_pred = 1; // b1
      if (b2_pred[global_pred] >= 2) {
        b2_hits += 1;
        b2_pred[global_pred] = 3;
      } else {
        b2_misses += 1;
        b2_pred[global_pred] += 1;
      }
      for (int j = 1; j < 11; j++) { // b2
        sum += i * j;
        if (b2_pred[global_pred] >= 2 && j != 10) {
          b2_hits += 1;
          b2_pred[global_pred] = 3;
        } else {
          b2_misses += 1;
          b2_pred[global_pred] += 1;
          if (j != 10) {
            global_pred = 1;
          }
        }
      }
    }
    if (b3_pred[global_pred] >= 2 && i != 100) {
      b3_hits += 1;
      b3_pred[global_pred] = 3;
    } else {
      b3_misses += 1;
      b3_pred[global_pred] += 1;
      if (i != 100) {
        global_pred = 1;
      }
    }
    sum += i;
  }

  cout << "b1 hits: " << b1_hits << "\nb1 misses: " << b1_misses << "\n";
  cout << "b2 hits: " << b2_hits << "\nb2 misses: " << b2_misses << "\n";
  cout << "b3 hits: " << b3_hits << "\nb3 misses: " << b3_misses << "\n";
}

void question_four(int b1_pred_init, int b2_pred_init, int b3_pred_init) {
  int b1_hits = 0;
  int b2_hits = 0;
  int b3_hits = 0;

  int b1_misses = 0;
  int b2_misses = 0;
  int b3_misses = 0;
  //(m,n) 2^m entries, n bit predictor. 2 bit global
  vector<int> b1_pred = {0, 0, 0, 0}; // 4 1 bit predictors
  vector<int> b2_pred = {0, 0, 0, 0};
  vector<int> b3_pred = {0, 0, 0, 0};
  int global_pred = 0b00; // 2 bit global

  int sum = 0;
  if (b3_pred[global_pred] < 2) {
    b3_misses += 1;
    b3_pred[global_pred] += 1;
  } else {
    b3_hits += 1;
    b3_pred[global_pred] = 3;
  }
  global_pred = 0b01;             // b3 will be taken
  for (int i = 1; i < 101; i++) { // b3
    if ((i % 4 == 0)) {
      if (b1_pred[global_pred] == 0) {
        b1_hits += 1;
        b1_pred[global_pred] = 0;
      } else {
        b1_misses += 1;
        b1_pred[global_pred] = 0;
      }
      if (global_pred != 0) {
        global_pred -= 1;
      }
    } else {
      if (b1_pred[global_pred] == 0) {
        b1_misses += 1;
        b1_pred[global_pred] = 1;
      } else {
        b1_hits += 1;
        b1_pred[global_pred] = 1;
      }
      if (global_pred != 0b11) {
        global_pred += 1;
        // b1 taken, increment
      }
    }
    if ((i % 4) == 0) { // b1
      if (b2_pred[global_pred] == 1) {
        b2_hits += 1;
      } else {
        b2_misses += 1;
      }
      for (int j = 1; j < 11; j++) { // 2
        sum += i * j;
        if (j != 10) {

          if (b2_pred[global_pred] == 1) {
            b2_hits += 1;
            b2_pred[global_pred] = 1;
          } else {
            b2_misses += 1;
            b2_pred[global_pred] = 1;
          }
        } else {
          b2_misses += 1;
          b2_pred[global_pred] = 0;
        }
        if (j != 10 && global_pred <= 2) {
          global_pred += 1;
        } else if (j == 10 && global_pred != 0b00) {
          global_pred -= 1;
        }
      }
    }
    sum += i;
    // cout << "b3[GLOBAL]: " << b3_pred[global_pred] << "\n";
    if (b3_pred[global_pred] == 1 && i != 100) {
      b3_hits += 1;
      b3_pred[global_pred] = 1;
    } else {
      b3_misses += 1;
      b3_pred[global_pred] = 1;
    }
    if (global_pred != 0b11) {
      global_pred += 1;
    }
  }
  cout << "b1 hits: " << b1_hits << "\nb1 misses: " << b1_misses << "\n";
  cout << "b2 hits: " << b2_hits << "\nb2 misses: " << b2_misses << "\n";
  cout << "b3 hits: " << b3_hits << "\nb3 misses: " << b3_misses << "\n";
}
