#include <iostream>

#include "gg.hpp"

void display(GDB & gdb, std::string & gdb_output, std::string & gdb_error) {
  // Clear string buffers 
  gdb_output.clear();
  gdb_error.clear();

  // Read from GDB to populate buffers
  gdb.read_until_prompt(gdb_output, gdb_error);

  if (!gdb_error.empty() || !gdb_output.empty()) {
    // Pass output to IO streams
    std::cerr << gdb_error << std::flush;
    std::cout << gdb_output << std::flush;
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

  // Display gdb introduction to user 
  display(gdb, gdb_output, gdb_error);

  while (gdb.is_running()) {
    // Clear input string buffer
    gdb_input.clear();

    // Read one line from stdin to process (blocking)
    std::getline(std::cin, gdb_input);

    // Exit if EOF detected in input stream
    if (std::cin.eof()) {
      std::cout << GDB_QUIT << std::endl;
      break;
    }

    // Handle empty input separately from regular command
    if (gdb_input.size()) {
      // Execute the command that we read in 
      gdb.execute(gdb_input);

      // Display the result of the command and the next prompt
      display(gdb, gdb_output, gdb_error);
    }
    else {
      // Display the prompt for the next command 
      std::cout << GDB_PROMPT;
    }
  }
}
