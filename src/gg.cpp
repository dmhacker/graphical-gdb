#include <iostream>
#include "gg.hpp"

void display_next(GDB & gdb, std::string & gdb_output, std::string & gdb_error) {
  // Clear string buffers 
  gdb_output.clear();
  gdb_error.clear();

  // Read from GDB to populate buffers
  gdb.read(gdb_output, gdb_error);

  // Pass output to IO streams
  std::cerr << gdb_error << std::flush;
  std::cout << gdb_output << std::flush;
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
  display_next(gdb, gdb_output, gdb_error);

  while (gdb.is_running()) {
    // Display the gdb prompt to the user
    display_next(gdb, gdb_output, gdb_error);

    // Clear input string buffer
    gdb_input.clear();

    // Read one line from stdin to process (blocking)
    std::getline(std::cin, gdb_input);

    // Execute the command that we read in
    gdb.execute(gdb_input);

    // Display result of command if we know that the command produces a result
    if (!gdb_input.empty()) {
      display_next(gdb, gdb_output, gdb_error);
    }
  }
}
