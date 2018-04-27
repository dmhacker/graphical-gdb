#include <iostream>
#include <sstream>

#include <wx/wx.h>

#include "pstream.hpp"

// Custom event types sent from the console to the GUI for updates.
const wxEventType gdbEVT_STATUS_BAR_UPDATE = wxNewEventType();
const wxEventType gdbEVT_SOURCE_CODE_UPDATE = wxNewEventType();
const wxEventType gdbEVT_ASSEMBLY_CODE_UPDATE = wxNewEventType();

// Class that represents a GDB process abstraction.
class GDB {
    redi::pstream process;
    char buf[BUFSIZ];
    std::streamsize bufsz;
    bool running_program;
    bool running_reset_flag;
    long saved_line_number;
  public:
    GDB(std::vector<std::string> args);
    ~GDB(void);
    void execute(const char * command);
    void read_until_prompt(std::ostream & output_buffer, std::ostream & error_buffer, bool trim_prompt);
    bool is_alive();
    bool is_running_program();
    std::string get_source_code();
    std::string get_assembly_code();
    long get_source_list_size();
    long get_source_line_number();
  private:
    void execute(const char * command, bool set_flags);
    std::string execute_and_read(const char * command);
    std::string execute_and_read(const char * command, long arg);
};

// Class representing the GDB GUI application.
class GDBApp : public wxApp {
  public:
    virtual bool OnInit();
};

// Class representing the GDB GUI top level display frame.
class GDBFrame : public wxFrame {
  wxStaticText * sourceCodeText;
  public:
    GDBFrame(const wxString & title, const wxPoint & pos, const wxSize & size);
  private:
    void OnAbout(wxCommandEvent & event);
    void OnExit(wxCommandEvent & event);
    void DoStatusBarUpdate(wxCommandEvent & event);
    void DoSourceCodeUpdate(wxCommandEvent & event);
    wxDECLARE_EVENT_TABLE();
};

// Macros used for binding events to wxWidgets frame functions.
wxBEGIN_EVENT_TABLE(GDBFrame, wxFrame)
  EVT_MENU(wxID_EXIT, GDBFrame::OnExit)
  EVT_MENU(wxID_ABOUT, GDBFrame::OnAbout)
  EVT_COMMAND(wxID_ANY, gdbEVT_STATUS_BAR_UPDATE, GDBFrame::DoStatusBarUpdate)
  EVT_COMMAND(wxID_ANY, gdbEVT_SOURCE_CODE_UPDATE, GDBFrame::DoSourceCodeUpdate)
wxEND_EVENT_TABLE()

// Macro to tell wxWidgets to use our GDB GUI application.
wxIMPLEMENT_APP_NO_MAIN(GDBApp);
