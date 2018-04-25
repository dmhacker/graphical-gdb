#include <thread>

#include <readline/readline.h>
#include <readline/history.h>

#include "gg.hpp"

void update_console_and_gui(GDB & gdb) {
  // Read from GDB to populate buffer
  gdb.read_until_prompt(std::cout, std::cerr, true);

  // Create event to update GUI status bar
  wxCommandEvent * sbu_event = 
    new wxCommandEvent(gdbEVT_STATUS_BAR_UPDATE);
  sbu_event->SetString(
      gdb.is_running_program() ? GDB_STATUS_RUNNING : GDB_STATUS_IDLE);

  // Queue events if application has been initialized on separate thread
  GDBApp * app = (GDBApp *) wxTheApp;
  wxWindow * window = app->GetTopWindow();
  if (window) { // Window will be null if GDBApp::OnInit() hasn't been called
    wxEvtHandler * window_handler = window->GetEventHandler();
    window_handler->QueueEvent(sbu_event);
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

  // Display gdb introduction to user 
  update_console_and_gui(gdb);

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
      update_console_and_gui(gdb);
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

