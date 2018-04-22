#include <iostream>
#include "pstream.hpp"

bool flush(redi::pstream & process, 
    char * buf, std::streamsize bufsz, 
    bool * cerr_eof, bool * cout_eof)
{
  bool written = false;

  // Write from process to stdout (non-blocking)
  if (!*cerr_eof) {
    while (bufsz = process.err().readsome(buf, sizeof(buf))) {
      std::cerr.write(buf, bufsz);
      written = true;
    }
    if (process.eof()) {
      *cerr_eof = true;
      if (!*cout_eof) {
        process.clear(); 
      }
    }
  }


  // Write from process to stdout (non-blocking)
  if (!*cout_eof) {
    while (bufsz = process.out().readsome(buf, sizeof(buf))) {
      std::cout.write(buf, bufsz);
      written = true;
    }
    if (process.eof()) {
      *cout_eof = true;
      if (!*cerr_eof) {
        process.clear(); 
      }
    }
  }

  return written;
}


int main(int argc, char ** argv) {
  // Convert char ** to std::vector<std::string>
  std::vector<std::string> argv_vector;
  for (int i = 0; i < argc; i++) {
    char * arg = argv[i];
    std::string arg_string(arg);
    argv_vector.push_back(arg_string);
  }

  // Open process process to gdb
  const redi::pstreams::pmode mode = 
    redi::pstreams::pstdin | 
    redi::pstreams::pstdout |
    redi::pstreams::pstderr;  
  redi::pstream process("gdb", argv_vector, mode);

  // Buffers, other variables 
  char buf[BUFSIZ];
  std::streamsize bufsz;
  std::string linebuf;
  bool cerr_eof = false;
  bool cout_eof = false;

  while(!flush(process, buf, bufsz, &cerr_eof, &cout_eof));

  while (!cerr_eof || !cout_eof) {
    // Read one line from stdin to process (blocking)
    if (!process.eof()) {
      std::cin >> linebuf;
      process << linebuf << std::endl;
    }

    while(!flush(process, buf, bufsz, &cerr_eof, &cout_eof));
  }
}


