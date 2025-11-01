CXX = g++
CXXFLAGS = -Wall -std=c++17 -g -Wno-sign-compare

SRC = main.cpp config.cpp
OBJ = $(SRC:.cpp=.o)
EXE = main

build: main.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(EXE) $(OBJ)

run: $(EXE)
	./$(EXE) < trace.dat

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJ)