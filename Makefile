CXX=g++
CXXFLAGS=-std=c++11
LDLIBS=`wx-config --cxxflags --libs` 

.PHONY: clean

all: build/gg build/simpletest
build/gg: src/gg.cpp src/gg.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(LDLIBS) include/* $<
build/simpletest: tests/simpletest.cpp
	$(CXX) $(CXXFLAGS) -o $@ -g $<
clean:
	rm -rf build/
