#include "pstream.hpp"

const redi::pstreams::pmode mode = 
  redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;

class GDB {
    redi::pstream process;
    char buf[BUFSIZ];
    std::streamsize bufsz;
  public:
    GDB(std::vector<std::string> argv_vector) :
      process("gdb", argv_vector, mode) {}

    ~GDB(void) {
      process.close();
    }

    void execute(std::string & command) {
      if (is_running()) {
        process << command << std::endl;
      }
    }

    void read(std::string & output, std::string & error) {
      while (is_running() && output.empty() && error.empty()) {
        try_read(output, error);
      }
    }

    bool is_running() {
      bool exited = 
        process.out().rdbuf()->exited() || 
        process.err().rdbuf()->exited();
      return !exited;
    }
  private:
    void try_read(std::string & output, std::string & error) {
      // Read process's error stream and append to error string
      while (bufsz = process.err().readsome(buf, sizeof(buf))) {
        error.append(buf, bufsz);
      }

      // Read process's output stream and append to output string 
      while (bufsz = process.out().readsome(buf, sizeof(buf))) {
        output.append(buf, bufsz);
      }
    }
};
