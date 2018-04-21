#include <iostream>
#include "pstream.hpp"

int main(int argc, char ** arcv) {
  const redi::pstreams::pmode mode = 
    redi::pstreams::pstdout|redi::pstreams::pstderr;  
  redi::ipstream child("echo OUT1; sleep 1; echo ERR >&2; sleep 1; echo OUT2", mode);
  char buf[1024];
  std::streamsize sz;
  bool finished[2] = { false, false };
  while (!finished[0] || !finished[1]) {
    if (!finished[0]) {
      while ((sz = child.err().readsome(buf, sizeof(buf))) > 0)
        std::cerr.write(buf, sz).flush();
      if (child.eof()) {
        finished[0] = true;
        if (!finished[1])
          child.clear();
      }
    }

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

