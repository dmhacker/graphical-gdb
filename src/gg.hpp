#include <wx/wx.h>

#include "pstream.hpp"

// Custom event types sent from the console to the GUI for updates.
const wxEventType GDB_EVT_STATUS_BAR_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_SOURCE_CODE_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_LOCALS_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_PARAMS_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_ASSEMBLY_CODE_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_REGISTERS_UPDATE = wxNewEventType();

// GDB process abstraction.
class GDB {
    redi::pstream process; // The bidirectional stream opened to the process
    char buf[BUFSIZ]; // Temporary buffer used to read output and error 
    std::streamsize bufsz; // Number of characters written to temporary buffer at a time
    bool running_program; // Cached value specifying if the user is debugging a program in GDB
    bool running_reset_flag; // Set to true when the value of running_program needs to be updated
    long saved_line_number; // The last known line we executed
  public:
    // Class constructor opens the process.
    GDB(std::vector<std::string> args);

    // Class desctructor closes the process.
    ~GDB(void);

    // Execute the given command by passing it to the process.
    void execute(const char * command);

    // Read whatever output and error is stored in the process.
    // Method will try executing non-blocking reads until ... 
    //  a) the GDB process quits
    //  b) the prompt is detected at the end of the output buffer
    void read_until_prompt(std::ostream & output_buffer, std::ostream & error_buffer, bool trim_prompt);

    // Returns true if the GDB process is still alive.
    bool is_alive();

    // Returns true if the GDB process is running/debugging a program.
    bool is_running_program();

    // Gets the source code around where GDB is positioned at.
    std::string get_source_code();
   
    // Gets the local variables in the function GDB is executing.
    std::string get_local_variables();
   
    // Gets the formal parameters (arguments) passed to the function GDB is executing.
    std::string get_formal_parameters();
    
    // Gets the assembly code for the function GDB is in.
    std::string get_assembly_code();

    // Gets the register values wherever GDB is stopped at.
    std::string get_registers();

    // Gets GDB's current source code list size.
    long get_source_list_size();
    
    // Gets the current line number GDB is positioned at.
    long get_source_line_number();

    // Gets the last line number GDB was positioned at.
    long get_saved_line_number() {
      return saved_line_number;
    }

    // Sets the last line number GDB was at.
    void set_saved_line_number(long line_number) {
      saved_line_number = line_number;
    }
  private:
    // Gives option to disable setting internal flags after an execution.
    void execute(const char * command, bool set_flags);

    // Error is merged with output, not recommended for normal use.
    std::string execute_and_read(const char * command);

    // Special case of execute_and_read with an integer argument. 
    std::string execute_and_read(const char * command, long arg);
};

// GUI application.
class GDBApp : public wxApp {
  public:
    // Called when our application is initialized via wxEntry().
    virtual bool OnInit();
};

// GUI display for source code, local variables, formal parameters.
class GDBSourcePanel : public wxPanel {
    wxTextCtrl * sourceCodeText; // Displays source code 
    wxTextCtrl * localsText; // Displays local variables
    wxTextCtrl * paramsText; // Displays formal parameters
  public:
    // Constructor for the panel.
    GDBSourcePanel(wxWindow * parent);

    // Sets the text of the source code display.
    void SetSourceCode(wxString value) {
      sourceCodeText->SetValue(value);
    }

    // Sets the text of the local variables display.
    void SetLocalVariables(wxString value) {
      localsText->SetValue(value);
    }

    // Sets the text of the formal parameters display.
    void SetFormalParameters(wxString value) {
      paramsText->SetValue(value);
    }
}; 

// GUI display for assembly code, registers, stack frame.
class GDBAssemblyPanel : public wxPanel {
    wxTextCtrl * assemblyCodeText; // Displays assembly code
    wxTextCtrl * registersText; // Displays register values
  public:
    // Constructor for the panel.
    GDBAssemblyPanel(wxWindow * parent);

    // Sets the text of the assembly code display.
    void SetAssemblyCode(wxString value) {
      assemblyCodeText->SetValue(value);
    }

    // Sets the text of the registers display.
    void SetRegisters(wxString value) {
      registersText->SetValue(value);
    }
};

// GUI top level display frame.
class GDBFrame : public wxFrame {
  wxString command;
  wxString args;
  GDBSourcePanel * sourcePanel;
  GDBAssemblyPanel * assemblyPanel;
  public:
    // Called by GDBApp::OnInit() when it is initializing the top level frame.
    GDBFrame(const wxString & title, 
        const wxString & clcommand, const wxString & clargs,
        const wxPoint & pos, const wxSize & size);
  private:
    // Called when the user clicks on the About button in the menu bar.
    void OnAbout(wxCommandEvent & event);

    // Called when the user quits the GUI.
    void OnExit(wxCommandEvent & event) {
      Close(true);
    }

    // Status bar should be updated.
    void DoStatusBarUpdate(wxCommandEvent & event) {
      SetStatusText(event.GetString());
    }
    
    // Source code display should be updated.
    void DoSourceCodeUpdate(wxCommandEvent & event) {
      sourcePanel->SetSourceCode(event.GetString());
    }

    // Local variable display should be updated.
    void DoLocalsUpdate(wxCommandEvent & event) {
      sourcePanel->SetLocalVariables(event.GetString());
    }

    // Formal parameter display should be updated.
    void DoParamsUpdate(wxCommandEvent & event) {
      sourcePanel->SetFormalParameters(event.GetString());
    }

    // Assembly code display should be updated.
    void DoAssemblyCodeUpdate(wxCommandEvent & event) {
      assemblyPanel->SetAssemblyCode(event.GetString());
    }

    // Registers display should be updated.
    void DoRegistersUpdate(wxCommandEvent & event) {
      assemblyPanel->SetRegisters(event.GetString());
    }

    // Macro to specify that this frame has events that need binding
    wxDECLARE_EVENT_TABLE();
};

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
wxEND_EVENT_TABLE()

// Macro to tell wxWidgets to use our GDB GUI application.
wxIMPLEMENT_APP_NO_MAIN(GDBApp);
