#include <wx/wx.h>

#include "pstream.hpp"

#define GDB_PROMPT "(gdb) "
#define GDB_QUIT "quit"

const redi::pstreams::pmode GDB_PMODE = 
  redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;

bool ends_with(std::string const & value, std::string const & ending) {
  if (ending.size() > value.size()) 
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

class GDB {
    redi::pstream process;
    char buf[BUFSIZ];
    std::streamsize bufsz;
  public:
    // Class constructor opens the process.
    GDB(std::vector<std::string> argv_vector) :
      process("gdb", argv_vector, GDB_PMODE) {}

    // Class desctructor closes the process.
    ~GDB(void) {
      process.close();
    }

    // Execute the given command by passing it to the process.
    void execute(const char * command) {
      if (command && is_running()) {
        process << command << std::endl;
      }
    }

    // Overloaded function to support std::strings in addition to C-style strings.
    void execute(const std::string & command) {
      execute(command.c_str());
    }

    // Read whatever output and error is stored in the process.
    // Method will try executing non-blocking reads until ... 
    //  a) the program quits
    //  b) it detects the prompt at the end of the stdout buffer
    void read_until_prompt(std::string & output, std::string & error, bool trim_prompt) {
      // Do non-blocking reads
      do {
        try_read(output, error);
      } while (is_running() && !ends_with(output, GDB_PROMPT));

      // Trim prompt from end if program is running and trim prompt is specified
      if (is_running() && trim_prompt) {
        output.erase(output.size() - strlen(GDB_PROMPT), output.size());
      }
    }

    // Returns true if the process is still running (e.g. it is expecting output).
    bool is_running() {
      bool exited = 
        process.out().rdbuf()->exited() || // Check process output buffer
        process.err().rdbuf()->exited();   // Check process error buffer
      return !exited;
    }
  private:
    // Performs a non-blocking read. 
    // Makes one pass over the process output/error streams to collect data. 
    void try_read(std::string & output, std::string & error) {
      // Read process's error stream and append to error string
      while (bufsz = process.err().readsome(buf, sizeof(buf))) {
        error.append(buf, bufsz);
      }

      // Read process's output stream and append to output string 
      while (bufsz = process.out().readsome(buf, sizeof(buf))) {
        output.append(buf, bufsz);
      }
    }
};

class GDBApp : public wxApp {
  public:
    virtual bool OnInit();
};

class GDBFrame : public wxFrame {
  public:
    GDBFrame(const wxString & title, const wxPoint & pos, const wxSize & size);
  private:
    void OnHello(wxCommandEvent & event);
    void OnAbout(wxCommandEvent & event);
    void OnExit(wxCommandEvent & event);
    wxDECLARE_EVENT_TABLE();
};

enum {
  ID_Hello = 1
};

bool GDBApp::OnInit() {
  long screen_x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
  long screen_y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
  long frame_x = screen_x / 2;
  long frame_y = screen_y / 2;
  GDBFrame * frame = new GDBFrame("GDB Display", 
      wxPoint((screen_x - frame_x) / 2, (screen_y - frame_y) / 2), 
      wxSize(frame_x, frame_y));
  frame->Show(true);
  return true;
}

GDBFrame::GDBFrame(const wxString & title, const wxPoint & pos, const wxSize & size) :
  wxFrame(NULL, wxID_ANY, title, pos, size) 
{
  wxMenu * menuFile = new wxMenu;
  menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
                             "Help string shown in status bar for this menu item");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu * menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar * menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");
  SetMenuBar(menuBar);

  CreateStatusBar();
  SetStatusText("Welcome to wxWidgets!");
}

void GDBFrame::OnHello(wxCommandEvent & event) {
  wxLogMessage("Hello world from wxWidgets!");
}

void GDBFrame::OnAbout(wxCommandEvent & event) {
  wxMessageBox("This is a wxWidget's Hello world sample",
                "About Hello World", wxOK | wxICON_INFORMATION);
}

void GDBFrame::OnExit(wxCommandEvent & event) {
  Close(true);
}

wxBEGIN_EVENT_TABLE(GDBFrame, wxFrame)
  EVT_MENU(ID_Hello, GDBFrame::OnHello)
  EVT_MENU(wxID_EXIT, GDBFrame::OnExit)
  EVT_MENU(wxID_ABOUT, GDBFrame::OnAbout)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP_NO_MAIN(GDBApp);


