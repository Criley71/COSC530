usage:
> make

if make doesnt work for some reason
  >
  g++ -Wall -std=c++17 -g -Wno-sign-compare -c main.cpp -o main.o
  g++ -Wall -std=c++17 -g -Wno-sign-compare -c dc.cpp -o dc.o
  g++ -Wall -std=c++17 -g -Wno-sign-compare -c l2.cpp -o l2.o
  g++ -Wall -std=c++17 -g -Wno-sign-compare -c pt.cpp -o pt.o
  g++ -Wall -std=c++17 -g -Wno-sign-compare -c dtlb.cpp -o dtlb.o
  g++ -Wall -std=c++17 -g -Wno-sign-compare -c config.cpp -o config.o
  g++ -Wall -std=c++17 -g -Wno-sign-compare -o main main.o dc.o l2.o pt.o dtlb.o config.o

run it
> ./main < trace.dat