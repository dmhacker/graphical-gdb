CXX=g++
CXXFLAGS=-std=c++11
WXLIBS=`wx-config --cxxflags --libs --static` 

.PHONY: clean

all: build/gg build/simpletest
build/gg: src/gg.cpp src/gg.hpp
	$(CXX) $(CXXFLAGS) -o $@ include/* $< $(WXLIBS)
build/simpletest: tests/simpletest.cpp
	$(CXX) $(CXXFLAGS) -o $@ -g $<
clean:
	rm -rf build/
