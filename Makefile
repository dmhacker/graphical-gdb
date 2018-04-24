CC=g++
FLAGS=-std=c++11
LIBS=-lreadline `wx-config --cxxflags --libs` 

.PHONY: clean

all: build/gg build/simpletest
build/gg: src/gg.cpp src/gg.hpp
	$(CC) $(FLAGS) $< -o $@ $(LIBS)
build/simpletest: tests/simpletest.cpp
	$(CC) $(FLAGS) $< -o $@ -g
clean:
	rm -rf build/
