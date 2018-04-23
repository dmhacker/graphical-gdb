#include <iostream>
#include <thread>

#include <readline/readline.h>
#include <readline/history.h>

#include "gg.hpp"

void write_console(GDB & gdb, std::string & gdb_output, std::string & gdb_error) {
  // Clear string buffers 
  gdb_output.clear();
  gdb_error.clear();

  // Read from GDB to populate buffers
  gdb.read_until_prompt(gdb_output, gdb_error, true);

  if (!gdb_error.empty() || !gdb_output.empty()) {
    // Pass output to IO streams
    std::cerr << gdb_error << std::flush;
    std::cout << gdb_output << std::flush;
  }
}

void open_console(std::vector<std::string> args) {
  // Create GDB process
  GDB gdb(args);

  // Create string buffers 
  std::string gdb_output;
  std::string gdb_error;

  // Display gdb introduction to user 
  write_console(gdb, gdb_output, gdb_error);

  while (gdb.is_running()) {
    // Read one line from stdin to process (blocking)
    char * buf_input = readline(GDB_PROMPT);

    // A null pointer signals input EOF 
    if (!buf_input) {
      std::cout << GDB_QUIT << std::endl;
      break;
    }

    // Handle empty input separately from regular command
    if (strlen(buf_input)) {
      // Add input to our CLI history
      add_history(buf_input);

      // Execute the command that we read in 
      std::string gdb_input(buf_input);
      gdb.execute(gdb_input);

      // Display the result of the command and the next prompt
      write_console(gdb, gdb_output, gdb_error);
    }
    else {
      // Display the prompt for the next command 
      std::cout << GDB_PROMPT;
    }

    // Delete the input buffer, free up memory
    delete buf_input;
  }
}

int main(int argc, char ** argv) {
  // Convert char ** to std::vector<std::string>
  std::vector<std::string> args;
  for (int i = 0; i < argc; i++) {
    char * arg = argv[i];
    std::string argstr(arg);
    args.push_back(argstr);
  }

  // Open console to accept user input in a separate thread
  std::thread console(open_console, args);

  // Run GUI on main thread
  wxEntry(argc, argv);

  return 0;
}

