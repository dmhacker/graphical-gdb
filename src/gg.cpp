#include <thread>
#include <iostream>
#include <sstream>

#include <readline/readline.h>
#include <readline/history.h> 
#include "gg.hpp" 

#define GDB_PROMPT "(gdb) " 
#define GDB_QUIT "quit"
#define GDB_JUMP "jump"
#define GDB_WHERE "where"
#define GDB_LIST "list" 
#define GDB_GET_LIST_SIZE "show listsize"
#define GDB_SET_LIST_SIZE "set listsize"
#define GDB_DISASSEMBLE "disassemble"
#define GDB_INFO_ARGUMENTS "info args"
#define GDB_INFO_LOCALS "info locals"
#define GDB_INFO_PROGRAM "info program"
#define GDB_TEMPORARY_BREAK "tbreak"

#define GDB_DISPLAY_LIST_SIZE 25

#define GDB_STATUS_IDLE "GDB is idle."
#define GDB_STATUS_RUNNING "GDB is currently running a program."
#define GDB_NO_SOURCE_CODE "No source code information available."
#define GDB_NO_LOCALS "No local variable information available."
#define GDB_NO_PARAMS "No parameter information available."

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
  std::string line = std::string(command) + " " + std::to_string(arg);
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
  // A running program has source code that can be printed
  if (is_running_program()) {
    // Save the current list size and list line number
    long list_size = get_source_list_size();
    long line_number = get_source_line_number();

    // Get source code lines
    execute_and_read(GDB_SET_LIST_SIZE, GDB_DISPLAY_LIST_SIZE);
    std::string source = execute_and_read(GDB_LIST, line_number);

    // Set a temporary breakpoint at the same line we are at and jump back
    // This resets the internal list flag and preserves where we are in terms of program execution
    if (line_number != saved_line_number) {
      execute_and_read(GDB_TEMPORARY_BREAK, line_number);
      execute_and_read(GDB_JUMP, line_number);
    }

    // Reset line size to what the user had originally
    execute_and_read(GDB_SET_LIST_SIZE, list_size);

    // Save the line number we were last at
    saved_line_number = line_number;

    return source; 
  }

  // Not relevant for programs that aren't running
  std::string empty;
  return empty;
}

std::string GDB::get_local_variables() {
  // A running program has local variables
  if (is_running_program()) {
    return execute_and_read(GDB_INFO_LOCALS);
  }
  
  // Not relevant for programs that aren't running
  std::string empty;
  return empty;
}

std::string GDB::get_formal_parameters() {
  // A running program has formal parameters 
  if (is_running_program()) {
    return execute_and_read(GDB_INFO_ARGUMENTS);
  }
  
  // Not relevant for programs that aren't running
  std::string empty;
  return empty;
}

std::string GDB::get_assembly_code() {
  // A running program has assembly that can be disassembled
  if (is_running_program()) {
    return execute_and_read(GDB_DISASSEMBLE);
  }
  
  // Not relevant for programs that aren't running
  std::string empty;
  return empty;
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

  // Create main frame and display 
  GDBFrame * frame = new GDBFrame("GDB Display", 
      wxPoint((screen_x - frame_x) / 2, (screen_y - frame_y) / 2), 
      wxSize(frame_x, frame_y));
  frame->Show(true);

  // Set top window to be the frame
  SetTopWindow(frame);

  return true;
}

GDBFrame::GDBFrame(const wxString & title, const wxPoint & pos, const wxSize & size) :
  wxFrame(NULL, wxID_ANY, title, pos, size) 
{
  // File section in the menu bar
  wxMenu * menuFile = new wxMenu;
  menuFile->Append(wxID_EXIT);

  // Help section in the menu bar
  wxMenu * menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT);

  // Menu bar on the top 
  wxMenuBar * menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");
  SetMenuBar(menuBar);

  // Create main panel and sizer
  wxScrolledWindow * panel = new wxScrolledWindow(this, wxID_ANY);
  wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

  // Style for future text boxes
  long textCtrlStyle = wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL | wxVSCROLL;

  // Create source code display and add to sizer
  sourceCodeText = new wxTextCtrl(panel, wxID_ANY, 
      wxT(GDB_NO_SOURCE_CODE),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(sourceCodeText, 1, wxALL | wxEXPAND, 5);

  // Add space
  sizer->AddSpacer(10);

  // Create local variables display and add to sizer
  localsText = new wxTextCtrl(panel, wxID_ANY, 
      wxT(GDB_NO_LOCALS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(localsText, 1, wxALL | wxEXPAND, 5);

  // Add space
  sizer->AddSpacer(10);

  // Create formal parameters display and add to sizer
  paramsText = new wxTextCtrl(panel, wxID_ANY, 
      wxT(GDB_NO_PARAMS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(paramsText, 1, wxALL | wxEXPAND, 5);

  // Set sizer to the panel
  panel->SetSizer(sizer);

  // Status bar on the bottom
  CreateStatusBar();
  SetStatusText(GDB_STATUS_IDLE);
}

void GDBFrame::OnAbout(wxCommandEvent & event) {
  wxMessageBox("This is a wxWidget's Hello world sample",
               "About Hello World", wxOK | wxICON_INFORMATION);
}

void GDBFrame::OnExit(wxCommandEvent & event) {
  Close(true);
}

void GDBFrame::DoStatusBarUpdate(wxCommandEvent & event) {
  SetStatusText(event.GetString());
}

void GDBFrame::DoSourceCodeUpdate(wxCommandEvent & event) {
  sourceCodeText->SetValue(event.GetString());
}

void GDBFrame::DoLocalsUpdate(wxCommandEvent & event) {
  localsText->SetValue(event.GetString());
}

void GDBFrame::DoParamsUpdate(wxCommandEvent & event) {
  paramsText->SetValue(event.GetString());
}

void update_console_and_gui(GDB & gdb) {
  // Read from GDB to populate buffer
  gdb.read_until_prompt(std::cout, std::cerr, true);

  // Queue events if gdb is alive and 
  // application has been initialized on separate thread
  if (gdb.is_alive() && wxTheApp) { // App will be null if wxEntry() hasn't been called
    GDBApp * app = (GDBApp *) wxTheApp;
    wxWindow * window = app->GetTopWindow();
    if (window) { // Window will be null if GDBApp::OnInit() hasn't been called
      wxEvtHandler * handler = window->GetEventHandler();

      // Create event objects
      wxCommandEvent * status_bar_update = 
        new wxCommandEvent(gdbEVT_STATUS_BAR_UPDATE);
      wxCommandEvent * source_code_update = 
        new wxCommandEvent(gdbEVT_SOURCE_CODE_UPDATE);
      wxCommandEvent * locals_update =
        new wxCommandEvent(gdbEVT_LOCALS_UPDATE);
      wxCommandEvent * params_update =
        new wxCommandEvent(gdbEVT_PARAMS_UPDATE);

      // Set contents of events
      std::string status_bar_message;
      std::string source_code_message;
      if (gdb.is_running_program()) {
        status_bar_update->SetString(GDB_STATUS_RUNNING);
        source_code_update->SetString(gdb.get_source_code());
        locals_update->SetString(gdb.get_local_variables());
        params_update->SetString(gdb.get_formal_parameters());
      }
      else {
        status_bar_update->SetString(GDB_STATUS_IDLE);
        source_code_update->SetString(GDB_NO_SOURCE_CODE);
        locals_update->SetString(GDB_NO_LOCALS);
        params_update->SetString(GDB_NO_PARAMS);
      }

      // Send events to GUI application
      handler->QueueEvent(status_bar_update);
      handler->QueueEvent(source_code_update); 
      handler->QueueEvent(locals_update);
      handler->QueueEvent(params_update);
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

  // Display gdb introduction to user 
  update_console_and_gui(gdb);

  while (gdb.is_alive()) {
    // Read one line from stdin to process (blocking)
    char * buf_input = readline(GDB_PROMPT);

    // A null pointer signals input EOF 
    if (!buf_input) {
      // Print quit prompt since it isn't printed from input 
      std::cout << GDB_QUIT << std::endl;

      // Execute the quit command
      gdb.execute(GDB_QUIT);

      // Display the result of the quit command
      update_console_and_gui(gdb);
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

