CXX=g++
CXXFLAGS=-std=c++11 `wx-config --cxxflags`

LIBS=-lreadline `wx-config --libs`
CLZS=src/gdb.cpp src/gui.cpp

.PHONY: clean

all: build/gg build/simpletest

build/gg: $(CLZS) 
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(CLZS) $(LIBS) -o $@

build/simpletest: tests/simpletest.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -g

clean:
	rm -rf build/

