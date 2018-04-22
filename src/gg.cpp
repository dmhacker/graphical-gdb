#include <iostream>
#include "gg.hpp"

#define MAX_INTRO_WRITES 3
#define MAX_COMMAND_WRITES 6
#define MAX_INTRO_TRIES 100000
#define MAX_COMMAND_TRIES 1000

void write_output(GDB & gdb, std::string & gdb_output, std::string & gdb_error, long times, long tries) {
  for (long i = 0; i < times; i++) {
    // Clear string buffers 
    gdb_output.clear();
    gdb_error.clear();

    // Read from GDB to populate buffers
    gdb.read_into(gdb_output, gdb_error, tries);

    // Pass output to IO streams
    if (!gdb_error.empty() || !gdb_output.empty()) {
      std::cerr << gdb_error << std::flush;
      std::cout << gdb_output << std::flush;
    }
  }
}

int main(int argc, char ** argv) {
  // Convert char ** to std::vector<std::string>
  std::vector<std::string> argv_vector;
  for (int i = 0; i < argc; i++) {
    char * arg = argv[i];
    std::string arg_string(arg);
    argv_vector.push_back(arg_string);
  }

  // Create GDB process
  GDB gdb(argv_vector);

  // Create string buffers 
  std::string gdb_input;
  std::string gdb_output;
  std::string gdb_error;

  // Perform initial flush to display gdb introduction to user 
  write_output(gdb, gdb_output, gdb_error, 1, MAX_INTRO_TRIES);

  // If file and other arguments provided to gdb, print results from those
  if (argv_vector.size()) {
    write_output(gdb, gdb_output, gdb_error, MAX_INTRO_WRITES, MAX_INTRO_TRIES);
  }

  while (gdb.is_running()) {
    // Display the gdb prompt to the user
    write_output(gdb, gdb_output, gdb_error, 1, MAX_COMMAND_TRIES);

    // Clear input string buffer
    gdb_input.clear();

    // Read one line from stdin to process (blocking)
    std::getline(std::cin, gdb_input);

    // Execute the command that we read in
    gdb.execute(gdb_input);

    // Display result of command 
    write_output(gdb, gdb_output, gdb_error, MAX_COMMAND_WRITES, MAX_COMMAND_TRIES);
  }
}
