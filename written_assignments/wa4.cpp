#include <iostream>
#include <vector>
using namespace std;

void question_one(int b1_pred_init, int b2_pred_init, int b3_pred_init);
void question_two(int b1_pred_init, int b2_pred_init, int b3_pred_init);

int main() {
  cout << "QUESTION 1" << "\n";
  question_one(1, 0, 0);
  cout << "\nQUESTION 2" << "\n";
  question_two(1, 2, 2);
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
    b3_misses++;
    b3_pred = 1;
  } else {
    b3_hits++;
  }
  for (int i = 1; i < 101; i++) {  // B3, outer loop condition: if (i < 101) continue to inner part of outer

    if (i % 4 == 0) {
      if (b1_pred) {
        b1_hits++;
      } else {
        b1_misses++;
        b1_pred = 1;
      }
    } else {
      if (b1_pred) {
        b1_misses++;
        b1_pred = 0;
      } else {
        b1_hits++;
      }
    }
    if ((i % 4) == 0) {    // B1, if ((i%4) != 0) skip the inner loop
      if (b2_pred == 0) {  // B2, inner loop entry prediction
        b2_misses++;
        b2_pred = 1;
      } else {
        b2_hits++;
      }
      for (int j = 1; j < 11; j++) {  // B2, inner loop condition: if (j < 11) continue inner loop

        sum += i * j;
        if (b2_pred && j != 10) {
          b2_hits++;
        } else {
          b2_misses++;
          b2_pred = 1;
        }
      }
    }
    sum += i;
    if (b3_pred && i != 100) {
      b3_hits++;
    } else {
      b3_misses++;
      b3_pred = 1;
    }
  }

  cout << "b1 hits: " << b1_hits << ", b1 misses: " << b1_misses << "\n";
  cout << "b2 hits: " << b2_hits << ", b2 misses: " << b2_misses << "\n";
  cout << "b3 hits: " << b3_hits << ", b3 misses: " << b3_misses << "\n";
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

  int b1_pred = b1_pred_init;  // 01
  int b2_pred = b2_pred_init;  // 10
  int b3_pred = b3_pred_init;  // 10

  int b1_hits = 0;
  int b2_hits = 0;
  int b3_hits = 0;

  int b1_misses = 0;
  int b2_misses = 0;
  int b3_misses = 0;
  // cout << "b3_pred " << b3_pred << "\n";

  if (b3_pred < 2) {
    b3_misses++;
    b3_pred = (b3_pred << 1) | 1;
    b3_pred &= 0b11;
  } else {
    b3_hits++;
    b3_pred |= 1;
    b3_pred &= 0b11;
  }
  for (int i = 1; i < 101; i++) {  // b3
    if (i % 4 == 0) {              // b1
      if (b1_pred == 0) {
        b1_misses++;
        b1_pred = 1;
      } else if (b1_pred == 1) {
        b1_misses++;
        b1_pred = 2;
      } else {
        if (b1_pred == 2) {
          b1_hits++;
          b1_pred = 3;
        } else {
          b1_hits++;
          b1_pred = 3;
        }
      }
    } else {
      if(b1_pred == 0) {
        b1_hits++;
        b1_pred = 0;
      } else if (b1_pred == 1) {
        b1_hits++;
        b1_pred = 0;
      } else if (b1_pred == 2) {
        b1_misses++;
        b1_pred = 1;
      } else if (b1_pred == 3) {
        b1_misses++;
        b1_pred = 2;
      }
    }
    if ((i % 4) == 0) {               // b1
      if(b2_pred < 2){
        b2_misses++;
        b2_pred +=1;
      } else {
        b2_hits++;
        b2_pred = 3;
      }
      for (int j = 1; j < 11; j++) {  // b2
        sum += i * j;

        if(b2_pred >=2 && j != 10){
          b2_hits++;
          b2_pred = 3;
        } else {
          b2_misses++;
          if(b2_pred != 0){
            b2_pred -=1;
          }
        }
      }
    }
    sum += i;
    // cout << "b3_pred " << b3_pred << "\n";
    if (b3_pred > 1 && i != 100) {
      b3_hits++;
      b3_pred = (b3_pred << 1) | 1;
      b3_pred &= 0b11;
    } else {
      b3_misses++;
      b3_pred = (b3_pred << 1) | 1;
      b3_pred &= 0b11;
    }
  }

  cout << "b1 hits: " << b1_hits << ", b1 misses: " << b1_misses << "\n";
  cout << "b2 hits: " << b2_hits << ", b2 misses: " << b2_misses << "\n";
  cout << "b3 hits: " << b3_hits << ", b3 misses: " << b3_misses << "\n";
}