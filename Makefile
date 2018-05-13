CXX=g++
CXXFLAGS=-std=c++11 `wx-config --cxxflags`
GGLIBS=-lreadline `wx-config --libs`

.PHONY: clean

all: build/gg build/simpletest
build/gg: src/gg.cpp src/gg.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< $(GGLIBS) -o $@
build/simpletest: tests/simpletest.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -g
clean:
	rm -rf build/

