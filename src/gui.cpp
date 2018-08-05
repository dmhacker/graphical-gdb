#include <wx/notebook.h>
#include <wx/gbsizer.h>
#include <wx/grid.h>
#include <wx/dataview.h>
#include <sstream>

#include "gg.hpp" 

std::string long_to_string(long value, int use_hex) {
  std::stringstream conversion;
  if (use_hex)
    conversion << "0x" << std::hex << value;
  else
    conversion << value;
  return conversion.str();
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

  // Create stack frame display
  stackPanel = new GDBStackPanel(tabs);
  tabs->AddPage(stackPanel, "Stack Frames");
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
  wxGridBagSizer * sizer = new wxGridBagSizer();
  SetSizer(sizer);

  // Style for future text boxes
  long textCtrlStyle = wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL | wxVSCROLL;

  // Create assembly code display and add to sizer
  assemblyCodeText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_ASSEMBLY_CODE),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(assemblyCodeText, wxGBPosition(0, 0), wxGBSpan(2, 1), wxALL | wxEXPAND, 5);

  // Create registers display and add to sizer
  registersText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_REGISTERS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(registersText, wxGBPosition(0, 1), wxGBSpan(2, 1), wxALL | wxEXPAND, 5);

  // Specify sizer rows and columns that should be growable
  sizer->AddGrowableRow(0, 1);
  sizer->AddGrowableCol(0, 1);
  sizer->AddGrowableRow(1, 1);
  sizer->AddGrowableCol(1, 1);
}

GDBStackPanel::GDBStackPanel(wxWindow * parent) : wxPanel(parent, wxID_ANY), stack_global(NULL), stack_size(0), stack_top(0) {
  // A simple box sizer should suffice
  wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
  SetSizer(sizer);

  // Create the grid object and the five columns that go with it
  grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
  grid->CreateGrid(0, 5);

  // Set the titles for each column
  grid->SetColLabelValue(0, "Address\t\t");
  grid->SetColLabelValue(1, "Address[0]\t\t");
  grid->SetColLabelValue(2, "Address[1]\t\t");
  grid->SetColLabelValue(3, "Address[2]\t\t");
  grid->SetColLabelValue(4, "Address[3]\t\t");

  // Disable editing & resize grid to fit labels
  grid->AutoSize();
  grid->EnableEditing(false);

  // Add the grid to the sizer
  sizer->Add(grid, 1, wxEXPAND | wxALL, 5);
}

void GDBStackPanel::SetStackFrame(MemoryLocation * stack_frame, long stack_frame_size) {
  // Delete old rows from the grid
  if (grid->GetNumberRows()) {
    grid->DeleteRows(0, grid->GetNumberRows());
  }

  // Add new rows if the stack frame size is greater than 0
  if (stack_frame_size) {
    long stack_pointer = stack_frame[0].address;
    long frame_pointer = stack_pointer + stack_frame_size;

    if (stack_global) {
      long stack_bottom = stack_top + stack_size;

      long new_stack_top = stack_pointer < stack_top ? stack_pointer : stack_top;
      long new_stack_bottom = frame_pointer > stack_bottom ? frame_pointer : stack_bottom;
      long new_stack_size = new_stack_bottom - new_stack_top;

      long * new_stack;
      int free_old_stack;
      if (new_stack_top == stack_top && new_stack_bottom == stack_bottom) {
        new_stack = stack_global;
        free_old_stack = 0;
      }
      else {
        new_stack = (long *) malloc(new_stack_size * sizeof(long));
        free_old_stack = 1;
      }

      for (int index = 0; index < new_stack_size; index++) {
        long address = new_stack_top + index;
        if (address >= stack_pointer && address < frame_pointer) {
          new_stack[index] = stack_frame[address - stack_pointer].value;
        }
        else if (address >= stack_top && address < stack_bottom) {
          new_stack[index] = stack_global[address - stack_top];
        }
        else {
          new_stack[index] = 0;
        }
      }

      if (free_old_stack) {
        delete stack_global;
      }

      stack_global = new_stack;
      stack_size = new_stack_size;
      stack_top = new_stack_top;
    }
    else {
      stack_global = (long *) malloc(stack_frame_size * sizeof(long));
      stack_size = stack_frame_size;
      stack_top = stack_frame[0].address;

      for (int index = 0; index < stack_frame_size; index++) {
        stack_global[index] = stack_frame[index].value;
      }
    }

    grid->AppendRows(stack_size / 4);

    // Loop through each value on the stack
    for (int index = 0; index < stack_size; index++) {
      // Fetch the stack value 
      long stack_value = stack_global[index];
      long stack_address = stack_top + index;

      // GUI positional arguments: where the value should be displayed 
      long row =  index / 4;
      long col = index % 4;

      // Set the row address & frame pointer offset
      if (col == 0) {
        grid->SetRowLabelValue(row, long_to_string(index - stack_frame_size, 0)); 
        grid->SetCellValue(row, 0, long_to_string(stack_top + index, 1)); 
        
        // Highlight the stack pointer
        if (stack_address == stack_pointer) {
          grid->SetCellBackgroundColour(row, 0, wxColour(255, 255, 124));
        }
        else if (stack_address == frame_pointer) {
          grid->SetCellBackgroundColour(row, 0, wxColour(182, 149, 192));
        }
      }

      // Set the cell value to be the stack value
      grid->SetCellValue(row, col + 1, long_to_string(stack_value, 1));
    }
  }
  else {
    if (stack_global) {
      delete stack_global;
      stack_global = nullptr;
      stack_size = 0;
      stack_top = 0;
    }
  }
}
