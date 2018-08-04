#include <iostream>
#include <sstream>
#include <iomanip>

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
  std::vector<MemoryLocation> stack_frame;

  // Program is not running
  if (!is_running_program()) {
    return stack_frame; 
  }

  std::string stack_pointer_output = 
    execute_and_read(GDB_PRINT, GDB_STACK_POINTER);
  std::string frame_pointer_output = 
    execute_and_read(GDB_PRINT, GDB_FRAME_POINTER);

  long start_offset = strlen(" (void *) ") + 1;
  long stack_start_index = stack_pointer_output.find('=') + start_offset;
  long frame_start_index = frame_pointer_output.find('=') + start_offset; 
  long stack_end_index = stack_pointer_output.find('\n');
  long frame_end_index = frame_pointer_output.find('\n');

  std::string stack_pointer_string =
    stack_pointer_output.substr(stack_start_index, stack_end_index);
  std::string frame_pointer_string =
    frame_pointer_output.substr(frame_start_index, frame_end_index);

  long stack_pointer = std::stol(stack_pointer_string, nullptr, 16);
  long frame_pointer = std::stol(frame_pointer_string, nullptr, 16);
  long stack_size = frame_pointer - stack_pointer;

  // Stack has negative size when main is finished
  if (stack_size < 0) {
    return stack_frame;
  }

  // TODO: populate stack frame vector
  std::cout << std::hex << std::setfill('0') << std::setw(2);
  std::cout << stack_pointer << std::endl;
  std::cout << frame_pointer << std::endl;
  std::cout << std::dec;
  std::cout << stack_size << std::endl;
  
  char examine[100];
  snprintf(examine, 100, "%s/%ld%s%s", GDB_EXAMINE, stack_size, GDB_MEMORY_SIZE_BYTE, GDB_MEMORY_TYPE_LONG);
  std::cout << examine << std::endl;
  std::string stack_frame_output = execute_and_read(examine, GDB_STACK_POINTER);
  
  std::cout << stack_frame_output << std::endl;

  return stack_frame;
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

  // Edge case: program can still be running but 
  // entering stdlib functions does not return line numbers
  if (output.find(':') == -1) {
    return 0;
  }

  std::string target_line = output.substr(output.find(':') + 1, output.size());
  std::string target_word = target_line.substr(0, target_line.find('\n'));
  return std::stol(target_word);
}
