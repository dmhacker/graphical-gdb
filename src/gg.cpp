#include <iostream>
#include "pstream.hpp"

int main(int argc, char ** argv) {
  // Convert char ** to std::vector<std::string>
  std::vector<std::string> argv_vector;
  for (int i = 0; i < argc; i++) {
    char * arg = argv[i];
    std::string arg_string(arg);
    argv_vector.push_back(arg_string);
  }

  // Open child process to gdb
  const redi::pstreams::pmode mode = 
    redi::pstreams::pstdout|redi::pstreams::pstderr;  
  redi::ipstream child("gdb", argv_vector, mode);

  // Buffers, other variables 
  char buf[1024];
  std::streamsize sz;
  bool finished[2] = { false, false };

  while (!finished[0] || !finished[1]) {
    // Process stderr (non-blocking)
    if (!finished[0]) {
      while ((sz = child.err().readsome(buf, sizeof(buf))) > 0)
        std::cerr.write(buf, sz).flush();
      if (child.eof()) {
        finished[0] = true;
        if (!finished[1])
          child.clear();
      }
    }
  
    // Process stdout (non-blocking)
    if (!finished[1]) {
      while ((sz = child.out().readsome(buf, sizeof(buf))) > 0)
        std::cout.write(buf, sz).flush();
      if (child.eof()) {
        finished[1] = true;
        if (!finished[0])
          child.clear();
      }
    }
  }
}

