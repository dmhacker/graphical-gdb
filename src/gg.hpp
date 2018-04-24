#include <wx/wx.h>

#include "pstream.hpp"

#define GDB_PROMPT "(gdb) "
#define GDB_QUIT "quit"

#define GDB_STATUS_IDLE "GDB is idle."
#define GDB_STATUS_RUNNING "GDB is currently running a program."

// GDB process mode uses its own stdin, stdout and stderr
const redi::pstreams::pmode GDB_PMODE = 
  redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;

// Helper function for determining if a string ends with a certain value
bool string_ends_with(std::string const & str, std::string const & ending) {
  if (ending.size() > str.size()) 
    return false;
  return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

// Helper function for determining if a string contains a value
bool string_contains(std::string const & str, std::string const & value) {
  return str.find(value) != std::string::npos;
}

// Struct to hold GDB output and indicate whether or not it is error output
typedef struct GDBOutput {
  std::string content;
  bool is_error;
} GDBOutput;

// GDB class that represents a process abstraction
class GDB {
    redi::pstream process;
    char buf[BUFSIZ];
    std::streamsize bufsz;
    std::vector<GDBOutput> buf_output;
  public:
    GDB(std::vector<std::string> args);
    ~GDB(void);
    void execute(const char * command);
    void read_until_prompt(std::vector<GDBOutput> & buffer, bool trim_prompt);
    bool is_alive();
    bool is_running_program();
  private:
    void try_read(std::vector<GDBOutput> & buffer);
};

// wxWidgets application class (serves as an interface with GDB)
class GDBApp : public wxApp {
  public:
    virtual bool OnInit();
};

// wxWidgets top level frame that goes with the GDB application
class GDBFrame : public wxFrame {
  public:
    GDBFrame(const wxString & title, const wxPoint & pos, const wxSize & size);
  private:
    void OnAbout(wxCommandEvent & event);
    void OnExit(wxCommandEvent & event);
    void DoStatusBarUpdate(wxCommandEvent & event);
    wxDECLARE_EVENT_TABLE();
};

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
void GDB::read_until_prompt(std::vector<GDBOutput> & buffer, bool trim_prompt) {
  // Do non-blocking reads
  do {
    try_read(buffer);
  } while (is_alive() && (buffer.empty() || !string_ends_with(buffer.back().content, GDB_PROMPT)));

  // Trim prompt from end if program is running and trim prompt is specified
  if (is_alive() && trim_prompt) {
    // Get the GDB output that contains the prompt and remove it from the buffer
    GDBOutput old_output = buffer.back();
    std::string prompt = old_output.content; 
    buffer.pop_back();

    // Discard last output if it contains only the prompt
    if (prompt.size() > strlen(GDB_PROMPT)) {
      // Erase part of the GDB output 
      prompt.erase(prompt.size() - strlen(GDB_PROMPT), prompt.size());

      // Add trimmed output back to the buffer
      GDBOutput trimmed_output { prompt, old_output.is_error };
      buffer.push_back(trimmed_output);
    }
  }
}

// Returns true if the GDB process is still alive 
bool GDB::is_alive() {
  bool exited = 
    process.out().rdbuf()->exited() || // Check process output buffer
    process.err().rdbuf()->exited();   // Check process error buffer
  return !exited;
}

// Returns true if the GDB process is running/debugging a program
bool GDB::is_running_program() {
  // Clear output buffer 
  buf_output.clear();
  
  // Call info program in GDB
  execute("info program");

  // Get result of command
  read_until_prompt(buf_output, true);

  // Output with "not being run" only appears when GDB is not running anything
  return !string_contains(buf_output.back().content, "not being run");
}

// Performs a non-blocking read. 
// Makes one pass over the process output/error streams to collect data. 
void GDB::try_read(std::vector<GDBOutput> & buffer) {
  // Read process's error stream and append to error string
  while (bufsz = process.err().readsome(buf, sizeof(buf))) {
    std::string error_string(buf, bufsz);
    GDBOutput error { error_string, true }; 
    buffer.push_back(error);
  }

  // Read process's output stream and append to output string 
  while (bufsz = process.out().readsome(buf, sizeof(buf))) {
    std::string output_string(buf, bufsz);
    GDBOutput output { output_string, false };
    buffer.push_back(output); 
  }
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
  wxMenu * menuFile = new wxMenu;
  menuFile->Append(wxID_EXIT);

  wxMenu * menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar * menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");
  SetMenuBar(menuBar);

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

wxBEGIN_EVENT_TABLE(GDBFrame, wxFrame)
  EVT_MENU(wxID_EXIT, GDBFrame::OnExit)
  EVT_MENU(wxID_ABOUT, GDBFrame::OnAbout)
  EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED, GDBFrame::DoStatusBarUpdate)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP_NO_MAIN(GDBApp);
