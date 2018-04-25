#include <iostream>
#include <sstream>

#include <wx/wx.h>

#include "pstream.hpp"

#define GDB_PROMPT "(gdb) "
#define GDB_QUIT "quit"

#define GDB_STATUS_IDLE "GDB is idle."
#define GDB_STATUS_RUNNING "GDB is currently running a program."

// GDB process mode should be completely self-contained. 
const redi::pstreams::pmode GDB_PMODE = 
  redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;

// Custom event types sent from the console to the GUI for updates.
const wxEventType gdbEVT_STATUS_BAR_UPDATE = wxNewEventType();

// Struct to hold GDB output and indicate if it is error output.
typedef struct GDBOutput {
  std::string content;
  bool is_error;
} GDBOutput;

// Class that represents a GDB process abstraction.
class GDB {
    redi::pstream process;
    char buf[BUFSIZ];
    std::streamsize bufsz;
    std::vector<GDBOutput> buf_output;
  public:
    GDB(std::vector<std::string> args);
    ~GDB(void);
    void execute(const char * command);
    void read_until_prompt(std::ostream & output_buffer, std::ostream & error_buffer, bool trim_prompt);
    bool is_alive();
    bool is_running_program();
};

// Class representing the GDB GUI application.
class GDBApp : public wxApp {
  public:
    virtual bool OnInit();
};

// Class representing the GDB GUI top level display frame.
class GDBFrame : public wxFrame {
  public:
    GDBFrame(const wxString & title, const wxPoint & pos, const wxSize & size);
  private:
    void OnAbout(wxCommandEvent & event);
    void OnExit(wxCommandEvent & event);
    void DoStatusBarUpdate(wxCommandEvent & event);
    wxDECLARE_EVENT_TABLE();
};

// Macros used for binding events to wxWidgets frame functions.
wxBEGIN_EVENT_TABLE(GDBFrame, wxFrame)
  EVT_MENU(wxID_EXIT, GDBFrame::OnExit)
  EVT_MENU(wxID_ABOUT, GDBFrame::OnAbout)
  EVT_COMMAND(wxID_ANY, gdbEVT_STATUS_BAR_UPDATE, GDBFrame::DoStatusBarUpdate)
wxEND_EVENT_TABLE()

// Macro to tell wxWidgets to use our GDB GUI application.
wxIMPLEMENT_APP_NO_MAIN(GDBApp);

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

// Class constructor opens the process.
GDB::GDB(std::vector<std::string> args) : 
  process("gdb", args, GDB_PMODE) {}

// Class desctructor closes the process.
GDB::~GDB() {
  process.close();
}

// Execute the given command by passing it to the process.
void GDB::execute(const char * line) {
  if (is_alive() && line) {
    // Pass line directly to process
    process << line << std::endl;
  }
}

// Read whatever output and error is stored in the process.
// Method will try executing non-blocking reads until ... 
//  a) the program quits
//  b) it detects the prompt at the end of the stdout buffer
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
    while (bufsz = process.out().readsome(buf, sizeof(buf))) {
      std::string output(buf, bufsz);

      // Signal a break if output ends with the prompt
      if (string_ends_with(output, GDB_PROMPT)) {
        hit_prompt = true;
        // Trim the prompt from the output if specified
        if (trim_prompt) {
          output.erase(output.size() - strlen(GDB_PROMPT), output.size());
        }
      }

      output_buffer << output << std::flush;
    }
  }
}

// Returns true if the GDB process is still alive.
bool GDB::is_alive() {
  bool exited = 
    process.out().rdbuf()->exited() || // Check process output buffer
    process.err().rdbuf()->exited();   // Check process error buffer
  return !exited;
}

// Returns true if the GDB process is running/debugging a program.
bool GDB::is_running_program() {
  // Create output stream buffer 
  // The buffer is also passed to error since the command produces no error
  std::ostringstream output_buffer;

  // Call info program in GDB
  execute("info program");

  // Get result of command
  read_until_prompt(output_buffer, output_buffer, true);

  // Output with "not being run" only appears when GDB is not running anything
  return !string_contains(output_buffer.str(), "not being run");
}

// Called when our application is initialized via wxEntry().
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

// Called by GDBApp::OnInit() when it is initializing the top level frame.
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

  // Status bar on the bottom
  CreateStatusBar();
  SetStatusText(GDB_STATUS_IDLE);
}

// Called when the user clicks on the About button in the menu bar.
void GDBFrame::OnAbout(wxCommandEvent & event) {
  wxMessageBox("This is a wxWidget's Hello world sample",
               "About Hello World", wxOK | wxICON_INFORMATION);
}

// Called when the user quits the GUI.
void GDBFrame::OnExit(wxCommandEvent & event) {
  Close(true);
}

// Called when then the console thread posts to the GUI thread
// that a status bar update should be made.
void GDBFrame::DoStatusBarUpdate(wxCommandEvent & event) {
  SetStatusText(event.GetString());
}
