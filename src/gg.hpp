#include "pstream.hpp"

const redi::pstreams::pmode mode = 
  redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;

class GDB {
  private:
    redi::pstream process;
    char buf[BUFSIZ];
    std::streamsize bufsz;
    std::string bufline;
  public:
    GDB(std::vector<std::string> argv_vector) :
      process("gdb", argv_vector, mode) {}

    ~GDB(void) {
      process.close();
    }

    void execute(std::string & command, std::string & output, std::string & error) {
      process << command << std::endl;
      flush(output, error);
    }

    void flush(std::string & output, std::string & error) {
      while (output.empty() && error.empty()) {
        try_flush(output, error);
      }
    }
  protected:
    void try_flush(std::string & output, std::string & error) {
      // Read process's output stream and append to output string 
      while (bufsz = process.out().readsome(buf, sizeof(buf))) {
        output.append(buf, bufsz);
      }

      // Read process's error stream and append to error string
      while (bufsz = process.err().readsome(buf, sizeof(buf))) {
        error.append(buf, bufsz);
      }
    }
};
