#include <thread>

#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>

#include "gg.hpp" 

// Macros used for binding events to wxWidgets frame functions.
wxBEGIN_EVENT_TABLE(GDBFrame, wxFrame)
  EVT_MENU(wxID_EXIT, GDBFrame::OnExit)
  EVT_MENU(wxID_ABOUT, GDBFrame::OnAbout)
  EVT_COMMAND(wxID_ANY, GDB_EVT_STATUS_BAR_UPDATE, GDBFrame::DoStatusBarUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_SOURCE_CODE_UPDATE, GDBFrame::DoSourceCodeUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_LOCALS_UPDATE, GDBFrame::DoLocalsUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_PARAMS_UPDATE, GDBFrame::DoParamsUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_ASSEMBLY_CODE_UPDATE, GDBFrame::DoAssemblyCodeUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_REGISTERS_UPDATE, GDBFrame::DoRegistersUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_STACK_FRAME_UPDATE, GDBFrame::DoStackFrameUpdate) 
wxEND_EVENT_TABLE()

// Macro to tell wxWidgets to use our GDB GUI application.
wxIMPLEMENT_APP_NO_MAIN(GDBApp);

void update_console_and_gui(GDB & gdb) {
  // Read from GDB to populate buffer
  gdb.read_until_prompt(std::cout, std::cerr, true);

  // Queue events if gdb is alive and 
  // application has been initialized on separate thread
  if (gdb.is_alive() && wxTheApp) { // App will be null if wxEntry() hasn't been called
    wxWindow * window = wxTheApp->GetTopWindow();
    if (window) { // Window will be null if GDBApp::OnInit() hasn't been called
      wxEvtHandler * handler = window->GetEventHandler();

      // Update displays if we detect line numbers have changed
      long line_number = gdb.is_running_program() ?
        gdb.get_source_line_number() : 0;
      long saved_line_number = gdb.get_saved_line_number();

      // Set saved line number to be current line number
      gdb.set_saved_line_number(line_number);

      if (line_number != saved_line_number) {
        // Create event objects
        wxCommandEvent * status_bar_update =  
          new wxCommandEvent(GDB_EVT_STATUS_BAR_UPDATE);
        wxCommandEvent * source_code_update = 
          new wxCommandEvent(GDB_EVT_SOURCE_CODE_UPDATE);
        wxCommandEvent * locals_update =
          new wxCommandEvent(GDB_EVT_LOCALS_UPDATE);
        wxCommandEvent * params_update =
          new wxCommandEvent(GDB_EVT_PARAMS_UPDATE);
        wxCommandEvent * assembly_code_update =
          new wxCommandEvent(GDB_EVT_ASSEMBLY_CODE_UPDATE);
        wxCommandEvent * registers_update =
          new wxCommandEvent(GDB_EVT_REGISTERS_UPDATE);
        wxCommandEvent * stack_frame_update =
          new wxCommandEvent(GDB_EVT_STACK_FRAME_UPDATE);

        // Set contents of events
        if (gdb.is_running_program()) {
          status_bar_update->SetString(GDB_STATUS_RUNNING);
          source_code_update->SetString(gdb.get_source_code());
          locals_update->SetString(gdb.get_local_variables());
          params_update->SetString(gdb.get_formal_parameters());
          assembly_code_update->SetString(gdb.get_assembly_code());
          registers_update->SetString(gdb.get_registers());

        }
        else {
          status_bar_update->SetString(GDB_STATUS_IDLE);
          source_code_update->SetString(GDB_NO_SOURCE_CODE);
          locals_update->SetString(GDB_NO_LOCALS);
          params_update->SetString(GDB_NO_PARAMS);
          assembly_code_update->SetString(GDB_NO_ASSEMBLY_CODE);
          registers_update->SetString(GDB_NO_REGISTERS);
        }

        std::vector<MemoryLocation> stack_frame = gdb.get_stack_frame();
        MemoryLocation * stack_frame_array = stack_frame.size() ? (MemoryLocation *) malloc(sizeof(MemoryLocation) * stack_frame.size()) : nullptr;
        for (int index = 0; index < stack_frame.size(); index++) {
          memcpy(stack_frame_array + index, &stack_frame[index], sizeof(MemoryLocation));
        }
        stack_frame_update->SetClientData(stack_frame_array);
        stack_frame_update->SetExtraLong(stack_frame.size());

        // Send events to GUI application
        handler->QueueEvent(status_bar_update);
        handler->QueueEvent(source_code_update); 
        handler->QueueEvent(locals_update);
        handler->QueueEvent(params_update);
        handler->QueueEvent(assembly_code_update);
        handler->QueueEvent(registers_update);
        handler->QueueEvent(stack_frame_update);
      }
    }
  }
}

void open_console(int argc, char ** argv) {
  // Convert raw C string to standard library string 
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

  // Keep track of last command executed 
  const char * last_command = nullptr; 

  // Set deletion flags
  bool last_command_deletion = true;
  bool final_command_deletion = true;

  while (gdb.is_alive()) {
    // Read one line from stdin to process (blocking)
    const char * command = readline(GDB_PROMPT);
    last_command_deletion = true;

    // A null pointer signals EOF and GDB should execute quit 
    if (!command) {
      // Print quit command
      std::cout << GDB_QUIT << std::endl;

      // Specify that the quit command should be executed
      command = GDB_QUIT; 

      // Do not delete the "quit" literal
      final_command_deletion = false;
    }

    // GDB handles empty commands by executing the previous command  
    if (!strlen(command)) {
      if (!last_command) {
        continue;
      }
      else {
        command = last_command;
        last_command_deletion = false;
      }
    }

    // Execute the command and display result
    gdb.execute(command);
    update_console_and_gui(gdb);

    // Add the command to history if user executed something different previously
    if (!last_command || strcmp(command, last_command)) {
      add_history(command);
    }

    // The current command becomes last command executed 
    if (last_command_deletion) {
      delete last_command;
      last_command = command;
    }
  }

  // Do final deletion - cleanup
  if (final_command_deletion) {
    delete last_command;
  }
}

void open_gui(int argc, char ** argv) {
  wxEntry(argc, argv);
}

int main(int argc, char ** argv) {
  // Run GUI on detached thread; main thread will post events to it
  std::thread gui(open_gui, argc, argv);
  gui.detach();

  // Main thread opens console to accept user input 
  open_console(argc, argv);

  return 0;
}
