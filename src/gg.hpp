#include "pstream.hpp"

#define MAX_READ_TRIES 100000

const redi::pstreams::pmode mode = 
  redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;

class GDB {
    redi::pstream process;
    char buf[BUFSIZ];
    std::streamsize bufsz;
  public:
    // Class constructor opens the process.
    GDB(std::vector<std::string> argv_vector) :
      process("gdb", argv_vector, mode) {}

    // Class desctructor closes the process.
    ~GDB(void) {
      process.close();
    }

    // Execute the given command.
    void execute(std::string & command) {
      if (is_running()) {
        // Run the command by passing it to the process
        process << command << std::endl;
      }
    }

    // Read whatever output and error is stored in the process.
    // Method will try executing non-blocking reads a maximum of {MAX_READ_TRIES} unless ...
    //  a) the program quits
    //  b) one of the output/error buffers becomes non-empty 
    void read_into(std::string & output, std::string & error) {
      // Keep executing non-blocking read until either program fails or buffers are not empty
      long counter = 0;
      while (counter < MAX_READ_TRIES && is_running() && output.empty() && error.empty()) {
        try_read(output, error);
        counter++;
      }
    }

    // Returns true if the process is still running (e.g. it is expecting output).
    bool is_running() {
      bool exited = 
        process.out().rdbuf()->exited() || // Check process output buffer
        process.err().rdbuf()->exited();   // Check process error buffer
      return !exited;
    }
  private:
    // Performs a non-blocking read. 
    // Makes one pass over the process output/error streams
    // to see if there is any data that needs to be read.
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
