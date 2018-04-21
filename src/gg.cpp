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
    redi::pstreams::pstdin | 
    redi::pstreams::pstdout |
    redi::pstreams::pstderr;  
  redi::pstream child("gdb", argv_vector, mode);

  // Buffers, other variables 
  char buf[BUFSIZ];
  std::streamsize bufsz;
  bool cerr_eof = false;
  bool cout_eof = false;

  while (!cerr_eof|| !cout_eof) {
    // Process stderr (non-blocking)
    if (!cerr_eof) {
      while ((bufsz = child.err().readsome(buf, sizeof(buf))) > 0) {
        std::cerr.write(buf, bufsz).flush();
      }
      if (child.eof()) {
        cerr_eof = true;
        if (!cout_eof) {
          child.clear();
        }
      }
    }
  
    // Process stdout (non-blocking)
    if (!cout_eof) {
      while ((bufsz = child.out().readsome(buf, sizeof(buf))) > 0) {
        std::cout.write(buf, bufsz).flush(); 
      }
      if (child.eof()) {
        cout_eof = true;
        if (!cerr_eof) {
          child.clear(); 
        }
      }
    }
  }
}

