CXX=g++
CXXFLAGS=-std=c++11

ifeq ($(ldflags), -static) 
	WXLIBS=`wx-config --cxxflags --libs -static` 
else
	WXLIBS=`wx-config --cxxflags --libs`
endif

.PHONY: clean

all: build/gg build/simpletest
build/gg: src/gg.cpp src/gg.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(WXLIBS) include/* $<
build/simpletest: tests/simpletest.cpp
	$(CXX) $(CXXFLAGS) -o $@ -g $<
clean:
	rm -rf build/
