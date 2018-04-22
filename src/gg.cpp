#include <iostream>
#include "gg.hpp"

void display_next(GDB & gdb, std::string & gdb_output, std::string & gdb_error) {
  // Clear output and error strings
  gdb_output.clear();
  gdb_error.clear();

  // Flush GDB, now strings are populated
  gdb.flush(gdb_output, gdb_error);

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

  while (true) {
    // Display the gdb prompt to the user
    display_next(gdb, gdb_output, gdb_error);

    // Clear string buffers 
    gdb_input.clear();
    gdb_output.clear(); 
    gdb_error.clear(); 

    // Read one line from stdin to process (blocking)
    std::getline(std::cin, gdb_input);

    // Execute the command that we read in
    gdb.execute(gdb_input, gdb_output, gdb_error);

    // Write output and error to their respective IO streams
    std::cerr << gdb_error << std::flush;
    std::cout << gdb_output << std::flush;
  }
}
