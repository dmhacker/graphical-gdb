#include "pstream.hpp"

#define GDB_PROMPT "(gdb) "
#define GDB_QUIT "quit"

const redi::pstreams::pmode GDB_PMODE = 
  redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;

bool ends_with(std::string const & value, std::string const & ending) {
  if (ending.size() > value.size()) 
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

class GDB {
    redi::pstream process;
    char buf[BUFSIZ];
    std::streamsize bufsz;
  public:
    // Class constructor opens the process.
    GDB(std::vector<std::string> argv_vector) :
      process("gdb", argv_vector, GDB_PMODE) {}

    // Class desctructor closes the process.
    ~GDB(void) {
      process.close();
    }

    // Execute the given command by passing it to the process.
    void execute(const char * command) {
      if (command && is_running()) {
        process << command << std::endl;
      }
    }

    // Overloaded function to support std::strings in addition to C-style strings.
    void execute(const std::string & command) {
      execute(command.c_str());
    }

    // Read whatever output and error is stored in the process.
    // Method will try executing non-blocking reads until ... 
    //  a) the program quits
    //  b) it detects the prompt at the end of the stdout buffer
    void read_until_prompt(std::string & output, std::string & error, bool trim_prompt) {
      // Do non-blocking reads
      do {
        try_read(output, error);
      } while (is_running() && !ends_with(output, GDB_PROMPT));

      // Trim prompt from end if program is running and trim prompt is specified
      if (is_running() && trim_prompt) {
        output.erase(output.size() - strlen(GDB_PROMPT), output.size());
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
    // Makes one pass over the process output/error streams to collect data. 
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
