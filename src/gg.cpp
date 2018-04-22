#include <iostream>
#include "gg.hpp"

#define MAX_READ_TRIES 1000

const std::string gdb_prompt = "(gdb) ";

void display(GDB & gdb, std::string & gdb_output, std::string & gdb_error) {
  do {
    // Break out of loop if process isn't running
    if (!gdb.is_running()) {
      break;
    }

    // Clear string buffers 
    gdb_output.clear();
    gdb_error.clear();

    // Read from GDB to populate buffers
    gdb.read_into(gdb_output, gdb_error, MAX_READ_TRIES);

    // Pass output to IO streams
    if (!gdb_error.empty() || !gdb_output.empty()) {
      std::cerr << gdb_error << std::flush;
      std::cout << gdb_output << std::flush;
    }
  } while (gdb_output.find(gdb_prompt) == std::string::npos);
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

  // Display gdb introduction to user 
  display(gdb, gdb_output, gdb_error);

  while (gdb.is_running()) {
    // Clear input string buffer
    gdb_input.clear();

    // Read one line from stdin to process (blocking)
    std::getline(std::cin, gdb_input);

    // Execute the command that we read in
    gdb.execute(gdb_input);

    // Display result of command and prompt for next command
    display(gdb, gdb_output, gdb_error);
  }
}
