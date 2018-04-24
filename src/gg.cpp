#include <iostream>
#include <thread>

#include <readline/readline.h>
#include <readline/history.h>

#include "gg.hpp"

void update_console_and_gui(GDB & gdb, std::vector<GDBOutput> & gdb_output) {
  // Clear output buffer
  gdb_output.clear();

  // Read from GDB to populate buffer
  gdb.read_until_prompt(gdb_output, true);

  // Create event to update GUI status bar
  wxCommandEvent * event = new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED);
  event->SetString(gdb.is_running_program() ? GDB_STATUS_RUNNING : GDB_STATUS_IDLE);

  // Queue event if application has been created
  GDBApp * app = (GDBApp *) wxTheApp;
  wxWindow * window = app->GetTopWindow();
  if (window) {
    wxEvtHandler * window_handler = window->GetEventHandler();
    if (window_handler) {
      window_handler->QueueEvent(event);
    }
  }

  // If there is output to be printed ... 
  if (!gdb_output.empty()) {
    // Iterate through every output line ...
    for (GDBOutput output : gdb_output) {
      // and pass it to its respective IO stream
      if (output.is_error) {
        std::cerr << output.content << std::flush;
      }
      else {
        std::cout << output.content << std::flush;
      }
    }
  }
}

void open_console(int argc, char ** argv) {
  // Convert raw C string to STL string 
  std::vector<std::string> args;
  for (int i = 0; i < argc; i++) {
    char * arg = argv[i];
    std::string argstr(arg);
    args.push_back(argstr);
  }

  // Create instance of GDB
  GDB gdb(args);

  // Create output buffer
  std::vector<GDBOutput> gdb_output;

  // Display gdb introduction to user 
  update_console_and_gui(gdb, gdb_output);

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
      update_console_and_gui(gdb, gdb_output);
    }

    // Delete the input buffer, free up memory
    delete buf_input;
  }
}

void open_gui(int argc, char ** argv) {
  wxEntry(argc, argv);
}

int main(int argc, char ** argv) {
  // Run GUI on detached thread; main thread will post events to it
  std::thread gui(open_gui, 1, argv);
  gui.detach();

  // Main thread opens console to accept user input 
  open_console(argc, argv);

  return 0;
}

