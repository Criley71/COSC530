#include <iostream>
#include <iomanip>
using namespace std;

int main(){
  int prev = 1;
  int hit = 0;
  int miss = 0;
  string con = "";
  cout << "p is predicted\nr is result\nh/m is hit miss\nif i%4==0 then its not taken(n)\nif i%4==1 then its taken(t)\n";
  cout << "  i | p | r | h/m\n----------------\n"; 
  for(int i = 1; i < 101; i++){
    if(prev == 1){
      con = "t";
    }else{
      con = "n";
    }
    cout << setw(3) << setfill(' ') << i  << " | " << con << " | ";
    if(i%4 == 0){
      cout << "n | ";
      if(prev == 0){
        hit+=1;
        cout << "hit\n";
      }else{
        miss+=1;
        cout << "miss\n";
      }
      prev = 0;
    }else{
      cout << "t | ";
      if(prev == 0){
        miss+=1;
        cout << "miss\n";
      }else{
        hit+=1;
        cout << "hit\n";
      }
      prev = 1;
    }
  
  }
  cout << "hit " << hit << " miss " << miss << "\n";
}