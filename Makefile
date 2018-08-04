CXX=g++
CXXFLAGS=-std=c++11 `wx-config --cxxflags`

LIBS=-lreadline `wx-config --libs`

OBJDIR=build/.objs

SRCS=src/gdb.cpp src/gui.cpp src/main.cpp
OBJS=$(patsubst src/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(SRCS)))

.PHONY: clean

all: build/gg build/simpletest

build/.sentinel: 
	mkdir -p $(OBJDIR) 
	touch $@

$(OBJDIR)/%.o: src/%.cpp src/gg.hpp build/.sentinel
	$(CXX) $(CXXFLAGS) -c $< $(LIBS) -o $@

build/gg: $(OBJS) 
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@

build/simpletest: tests/simpletest.cpp build/.sentinel
	$(CXX) $(CXXFLAGS) $< -o $@ -g

clean:
	rm -rf build/

