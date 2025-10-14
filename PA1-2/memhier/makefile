CXX = g++
CXXFLAGS = -Wall -std=c++17 -g -Wno-sign-compare

SRC = main.cpp dc.cpp l2.cpp pt.cpp dtlb.cpp config.cpp
OBJ = $(SRC:.cpp=.o)
EXE = memhier

build: main.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(EXE) $(OBJ)

run: $(EXE)
	./$(EXE) < test.dat

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJ)
