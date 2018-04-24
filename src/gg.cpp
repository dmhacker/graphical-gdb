#include <iostream>
#include <thread>

#include <readline/readline.h>
#include <readline/history.h>

#include "gg.hpp"

void update_console_and_gui(GDB & gdb, std::string & gdb_output, std::string & gdb_error) {
  // Clear string buffers 
  gdb_output.clear();
  gdb_error.clear();

  // Read from GDB to populate buffers
  gdb.read_until_prompt(gdb_output, gdb_error, true);

  // Update GUI status bar
  wxCommandEvent * event = new wxCommandEvent(wxEVT_NULL, ID_STATUS_BAR_UPDATE);
  event->SetString(gdb.is_running_program() ? GDB_STATUS_RUNNING : GDB_STATUS_IDLE);
  wxTheApp->QueueEvent(event);

  if (!gdb_error.empty() || !gdb_output.empty()) {
    // Pass output to IO streams
    std::cerr << gdb_error << std::flush;
    std::cout << gdb_output << std::flush;
  }
}

void open_console(GDB & gdb) {
    // Create string buffers 
  std::string gdb_output;
  std::string gdb_error;

  // Display gdb introduction to user 
  update_console_and_gui(gdb, gdb_output, gdb_error);

  while (gdb.is_alive()) {
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
      gdb.execute(buf_input);

      // Display the result of the command and the next prompt
      update_console_and_gui(gdb, gdb_output, gdb_error);
    }

    // Delete the input buffer, free up memory
    delete buf_input;
  }
}

void open_gui() {
  // Pass in empty arguments
  int wx_argc = 0;
  char ** wx_argv = nullptr;

  // Open GUI using wxEntry
  wxEntry(wx_argc, wx_argv);
}

int main(int argc, char ** argv) {
  // Convert char ** to std::vector<std::string>
  std::vector<std::string> args;
  for (int i = 0; i < argc; i++) {
    char * arg = argv[i];
    std::string argstr(arg);
    args.push_back(argstr);
  }

  // Create GDB process
  GDB gdb(args);

  // Run GUI on detached thread; main thread will post events to it
  std::thread gui(open_gui);
  gui.detach();

  // Main thread opens console to accept user input 
  open_console(gdb);

  return 0;
}

