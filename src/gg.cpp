#include <thread>
#include <iostream>
#include <sstream>

#include <readline/readline.h>
#include <readline/history.h>

#include <wx/notebook.h>
#include <wx/gbsizer.h>

#include "gg.hpp" 

// Helper function for determining if a string ends with a certain value.
bool string_ends_with(std::string const & str, std::string const & ending) {
  if (ending.size() > str.size()) 
    return false;
  return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

// Helper function for determining if a string contains a value.
bool string_contains(std::string const & str, std::string const & value) {
  return str.find(value) != std::string::npos;
}

GDB::GDB(std::vector<std::string> args) : 
  process("gdb", args, 
      redi::pstreams::pstdin | 
      redi::pstreams::pstdout | 
      redi::pstreams::pstderr), 
  saved_line_number(0),
  running_reset_flag(false), 
  running_program(false) {}

GDB::~GDB() {
  process.close();
}

void GDB::execute(const char * command) {
  execute(command, true);
}

void GDB::execute(const char * command, bool set_flags) {
  if (is_alive() && command) {
    // Pass line directly to process
    process << command << std::endl;

    // Mark reset flag for running program
    running_reset_flag = set_flags;
  }
}

std::string GDB::execute_and_read(const char * command) {
  // Call line in GDB 
  execute(command, false);  

  // Create stream buffer
  std::ostringstream buffer;

  // Get result of command
  read_until_prompt(buffer, buffer, true);

  return buffer.str();
}

std::string GDB::execute_and_read(const char * command, long arg) {
  // e.g. line = "set listize 10"
  std::string line = std::string(command) + " " + std::to_string(arg);

  return execute_and_read(line.c_str());
}

std::string GDB::execute_and_read(const char * command, const char * arg) {
  // e.g. line = "print $sp"
  std::string line = std::string(command) + " " + std::string(arg);

  return execute_and_read(line.c_str());
}

std::string GDB::examine_and_read(const char * memory_location,
    const char * memory_type, long num_addresses) 
{
  // e.g. line = "x/24x $sp"
  std::string line = std::string(GDB_EXAMINE) + 
    "/" + std::to_string(num_addresses) + std::string(memory_type) +
    " " + memory_location;

  return execute_and_read(line.c_str());
}

void GDB::read_until_prompt(std::ostream & output_buffer, std::ostream & error_buffer, bool trim_prompt) {
  // Do non-blocking reads
  bool hit_prompt = false;
  while (is_alive() && !hit_prompt) {
    // Read process's error stream and append to error string
    while (bufsz = process.err().readsome(buf, sizeof(buf))) {
      std::string error(buf, bufsz);
      error_buffer << error << std::flush;
    }

    // Read process's output stream and append to output string 
    std::string last_output; // Intermediate buffer used to hold last line of output
    while (bufsz = process.out().readsome(buf, sizeof(buf))) {
      std::string output(buf, bufsz);

      // Signal a break if output ends with the prompt
      std::string combined_output = last_output + output; // Prompt can be split between two lines 
      if (string_ends_with(combined_output, GDB_PROMPT)) {
        hit_prompt = true;

        // Trim the prompt from the output if specified
        if (trim_prompt) {
          combined_output.erase(combined_output.size() - strlen(GDB_PROMPT), combined_output.size());
        }
        
        // Next output to print should be the combined output (prevents double printing)
        last_output = combined_output;
      }
      else {
        // Flush last output
        output_buffer << last_output << std::flush;

        // Set next output to print to be the current output
        last_output = output;
      }
    }

    // Flush last output that wasn't emptied by the loop
    output_buffer << last_output << std::flush;
  }
}

bool GDB::is_alive() {
  bool exited = 
    process.out().rdbuf()->exited() || // Check process output buffer
    process.err().rdbuf()->exited();   // Check process error buffer
  return !exited;
}

bool GDB::is_running_program() {
  if (running_reset_flag) {
    // Collect program status output
    std::string program_status = execute_and_read(GDB_INFO_PROGRAM);

    // Output with "not being run" only appears when GDB is not running anything
    running_program = !string_contains(program_status, "not being run");

    // Set flag to false, execute will reset it
    running_reset_flag = false;
  }

  return running_program; 
}

std::string GDB::get_source_code() {
  // Program is not running
  if (!is_running_program()) {
    return std::string(GDB_NO_SOURCE_CODE);
  }

  // Save the current list size and list line number
  long list_size = get_source_list_size();

  // Get source code lines
  execute_and_read(GDB_SET_LIST_SIZE, GG_FRAME_LINES);
  std::string source = execute_and_read(GDB_LIST, saved_line_number); 

  // Trick GDB's list range by printing the line before what we printed
  // NOT A PERFECT SOLUTION: if original_line_number == 1, then the first
  // line will never be printed by a subsequent list call
  long original_line_number = std::max(
      (long) 1, saved_line_number - list_size / 2 - 1);
  execute_and_read(GDB_SET_LIST_SIZE, 1);
  execute_and_read(GDB_LIST, original_line_number);

  // Restore old list size
  execute_and_read(GDB_SET_LIST_SIZE, list_size);

  return source; 
}

std::string GDB::get_local_variables() {
  // Program is not running
  if (!is_running_program()) {
    return std::string(GDB_NO_LOCALS);
  }

  return execute_and_read(GDB_INFO_LOCALS);
}

std::string GDB::get_formal_parameters() {
  // Program is not running
  if (!is_running_program()) {
    return std::string(GDB_NO_PARAMS);
  }

  return execute_and_read(GDB_INFO_ARGUMENTS);
}

std::string GDB::get_variable_value(const char * variable) {
  // Program is not running
  if (!is_running_program()) {
    return std::string(GDB_NO_VARIABLE);
  }

  // Get raw value of variable (e.g. $1 = ...)
  std::string value = execute_and_read(GDB_PRINT, variable);

  // Return original statement if variable isn't present
  long split_index = value.find('=');
  if (split_index == std::string::npos) {
    return value;
  }

  // Trim first part of the statement to get actual value
  return value.substr(split_index + 2, value.size());
}

std::vector<MemoryLocation> GDB::get_stack_frame() {
  // Program is not running
  if (!is_running_program()) {
    return std::vector<MemoryLocation>();
  }
}

std::string GDB::get_assembly_code() {
  // Program is not running
  if (!is_running_program()) {
    return std::string(GDB_NO_ASSEMBLY_CODE);
  }

  // Get full assembly dump
  std::string assembly_dump = execute_and_read(GDB_DISASSEMBLE);
  std::stringstream assembly_stream(assembly_dump);

  // Vector holding split lines 
  std::vector<std::string> assembly_lines; 
  // Buffer used to hold a line 
  std::string buffer; 
  // Index of the line we are looking at
  int current_line = 0; 
  // Index of the line GDB is executing
  int executing_line = 0; 

  // Break assembly dump into separate lines and determine executing line
  while (std::getline(assembly_stream, buffer, '\n')) {
    assembly_lines.push_back(buffer);

    // Executing assembly line contains a specific substring 
    if (string_contains(buffer, "=>")) {
      executing_line = current_line;
    }

    current_line++;
  }

  // Concise assembly string that we want to return
  std::string assembly;
  // Relevant starting line in the assembly dump 
  int starting_line = std::max(1, executing_line - GG_FRAME_LINES / 2);
  // Relevant ending line in the assembly dump
  int ending_line = starting_line + GG_FRAME_LINES;

  // Iterate through all relevant lines and append each to output 
  for (int i = starting_line; i < ending_line; i++) {
    if (i < assembly_lines.size()) {
      assembly.append(assembly_lines[i]).append("\n");
    }
  }

  return assembly;
}

std::string GDB::get_registers() {
  // Program is not running
  if (!is_running_program()) {
    return std::string(GDB_NO_REGISTERS);
  }

  return execute_and_read(GDB_INFO_REGISTERS);
}

long GDB::get_source_list_size() {
  std::string output = execute_and_read(GDB_GET_LIST_SIZE);
  std::string last_word = output.substr(output.find_last_of(' '), output.size() - 1);
  return std::stol(last_word); 
}

long GDB::get_source_line_number() {
  std::string output = execute_and_read(GDB_WHERE);
  std::string target_line = output.substr(output.find(':') + 1, output.size());
  std::string target_word = target_line.substr(0, target_line.find('\n'));
  return std::stol(target_word);
}

bool GDBApp::OnInit() {
  // Determine screen and application dimensions
  long screen_x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
  long screen_y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
  long frame_x = screen_x / 2;
  long frame_y = screen_y / 2;

  // Build command used to initialize application 
  std::string args;
  for (int i = 1; i < wxApp::argc; i++) {
    args.append(wxApp::argv[i]);
    if (i < wxApp::argc - 1) {
      args.append(" ");
    }
  }

  // Create main frame and display 
  GDBFrame * frame = new GDBFrame(GG_FRAME_TITLE, wxApp::argv[0], args, 
      wxPoint((screen_x - frame_x) / 2, (screen_y - frame_y) / 2), 
      wxSize(frame_x, frame_y));
  frame->Show(true);

  // Set top window to be the frame
  SetTopWindow(frame);

  return true;
}

GDBFrame::GDBFrame(const wxString & title, 
    const wxString & clcommand, const wxString & clargs,
    const wxPoint & pos, const wxSize & size) :
  wxFrame(NULL, wxID_ANY, title, pos, size), 
  command(clcommand), args(clargs)
{
  // File section in the menu bar
  wxMenu * menuFile = new wxMenu();
  menuFile->Append(wxID_EXIT);

  // Help section in the menu bar
  wxMenu * menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);

  // Menu bar on the top 
  wxMenuBar * menuBar = new wxMenuBar();
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");
  SetMenuBar(menuBar);

  // Status bar on the bottom
  CreateStatusBar();
  SetStatusText(GDB_STATUS_IDLE);

  // Create notebook (tabbed pane)
  wxNotebook * tabs = new wxNotebook(this, wxID_ANY);

  // Create source code display 
  sourcePanel = new GDBSourcePanel(tabs);
  tabs->AddPage(sourcePanel, "Source");

  // Create assembly code display
  assemblyPanel = new GDBAssemblyPanel(tabs);
  tabs->AddPage(assemblyPanel, "Assembly");
}

void GDBFrame::OnAbout(wxCommandEvent & event) {
  // Display static information
  const char * information = 
    "\nVersion: v"
    GG_VERSION
    "\nAuthors: "
    GG_AUTHORS
    "\nLicense: "
    GG_LICENSE;

  // Display per-instance information 
  std::string text(information);
  text.append("\n\nCommand: ");
  text.append(command.c_str());
  text.append("\nArguments: ");
  text.append(args.c_str());

  // Show message box with the text
  wxMessageBox(text, GG_ABOUT_TITLE, wxOK | wxICON_INFORMATION);
}

GDBSourcePanel::GDBSourcePanel(wxWindow * parent) :
  wxPanel(parent, wxID_ANY) 
{
  // Create main sizer
  wxGridBagSizer * sizer = new wxGridBagSizer();
  SetSizer(sizer);

  // Style for future text boxes
  long textCtrlStyle = wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL | wxVSCROLL;

  // Create source code display and add to sizer
  sourceCodeText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_SOURCE_CODE),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(sourceCodeText, 
      wxGBPosition(0, 0), wxGBSpan(2, 1), 
      wxALL | wxEXPAND, 5);

  // Create local variables display and add to sizer
  localsText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_LOCALS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(localsText, 
      wxGBPosition(0, 1), wxGBSpan(1, 1), 
      wxALL | wxEXPAND, 5);

  // Create formal parameters display and add to sizer
  paramsText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_PARAMS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(paramsText, 
      wxGBPosition(1, 1), wxGBSpan(1, 1), 
      wxALL | wxEXPAND, 5);

  // Specify sizer rows and columns that should be growable
  for (int i = 0; i < 2; i++) {
    sizer->AddGrowableRow(i, 1);
    sizer->AddGrowableCol(i, 1);
  }
}

GDBAssemblyPanel::GDBAssemblyPanel(wxWindow * parent) :
  wxPanel(parent, wxID_ANY) 
{
   // Create sizer
  wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
  SetSizer(sizer);

  // Style for future text boxes
  long textCtrlStyle = wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL | wxVSCROLL;

  // Create assembly code display and add to sizer
  assemblyCodeText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_ASSEMBLY_CODE),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(assemblyCodeText, 1, wxALL | wxEXPAND, 5);

  // Create registers display and add to sizer
  registersText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_REGISTERS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(registersText, 1, wxALL | wxEXPAND, 5);
}

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

        // Send events to GUI application
        handler->QueueEvent(status_bar_update);
        handler->QueueEvent(source_code_update); 
        handler->QueueEvent(locals_update);
        handler->QueueEvent(params_update);
        handler->QueueEvent(assembly_code_update);
        handler->QueueEvent(registers_update);
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
