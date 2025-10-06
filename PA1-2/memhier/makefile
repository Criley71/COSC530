CXX = g++
CXXFLAGS = -Wall -std=c++17 -g -Wno-sign-compare

SRC = main.cpp dc.cpp l2.cpp pt.cpp dtlb.cpp config.cpp
OBJ = $(SRC:.cpp=.o)
EXE = main

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJ)
