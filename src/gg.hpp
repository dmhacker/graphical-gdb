#include <wx/wx.h>
#include <wx/grid.h>

#include "../include/pstream.hpp"

#define GG_FRAME_TITLE "GDB Display"
#define GG_ABOUT_TITLE "About GG"
#define GG_VERSION "0.0.1"
#define GG_AUTHORS "David Hacker"
#define GG_LICENSE "GNU GPL v3.0"

#define GG_FRAME_LINES 19
#define GG_HISTORY_MAX_LENGTH 1000

#define GDB_PROMPT "(gdb) " 
#define GDB_QUIT "quit"
#define GDB_WHERE "where"
#define GDB_LIST "list" 
#define GDB_GET_LIST_SIZE "show listsize"
#define GDB_SET_LIST_SIZE "set listsize"
#define GDB_DISASSEMBLE "disassemble"
#define GDB_INFO_ARGUMENTS "info args"
#define GDB_INFO_LOCALS "info locals"
#define GDB_INFO_PROGRAM "info program"
#define GDB_INFO_REGISTERS "info registers"
#define GDB_PRINT "p"
#define GDB_EXAMINE "x"

#define GDB_STACK_POINTER "$sp"
#define GDB_FRAME_POINTER "$fp"

#define GDB_MEMORY_TYPE_LONG "x"
#define GDB_MEMORY_TYPE_INSTRUCTION "i"

#define GDB_STATUS_IDLE "GDB is idle."
#define GDB_STATUS_RUNNING "GDB is currently running a program."
#define GDB_NO_SOURCE_CODE "No source code information available."
#define GDB_NO_LOCALS "No local variable information available."
#define GDB_NO_PARAMS "No parameter information available."
#define GDB_NO_VARIABLE "No variable information available."
#define GDB_NO_ASSEMBLY_CODE "No assembly code information available."
#define GDB_NO_REGISTERS "No register information available."

// Custom event types sent from the console to the GUI for updates.
const wxEventType GDB_EVT_STATUS_BAR_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_SOURCE_CODE_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_LOCALS_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_PARAMS_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_ASSEMBLY_CODE_UPDATE = wxNewEventType();
const wxEventType GDB_EVT_REGISTERS_UPDATE = wxNewEventType();

// Represents a location in memory.
struct MemoryLocation {
  std::string address;
  std::string value;
};

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

  // Gets the value of a variable.
  std::string get_variable_value(const char * variable);

  // Gets the value of the current stack frame.
  std::vector<MemoryLocation> get_stack_frame();

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

  // Special case of execute_and_read with a string argument.
  std::string execute_and_read(const char * command, const char * arg);

  // Examines the memory at the given location.
  std::string examine_and_read(const char * memory_location, 
      const char * memory_type, long num_addresses);
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

// GUI display for assembly code & registers
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

// GUI display for stack frame
class GDBStackPanel : public wxPanel {
  wxGrid * grid;
  public:
  GDBStackPanel(wxWindow * parent);
};

// GUI top level display frame.
class GDBFrame : public wxFrame {
  wxString command;
  wxString args;
  GDBSourcePanel * sourcePanel;
  GDBAssemblyPanel * assemblyPanel;
  GDBStackPanel * stackPanel;
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
