# Makefile for cacheSim

CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall

# Target: build cacheSim from single source file
cacheSim: cacheSim.cpp
	$(CXX) $(CXXFLAGS) cacheSim.cpp -o cacheSim

.PHONY: clean
clean:
	rm -f cacheSim

