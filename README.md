# graphical-gdb

<a><img src="https://github.com/dmhacker/graphical-gdb/blob/master/images/terminal.png" align="center" height="323" width="447"></a>
<a><img src="https://github.com/dmhacker/graphical-gdb/blob/master/images/gui.png" align="center" height="400" width="425"></a>

A wrapper around GDB that provides you with a visual of your program's current state. 

Running this program opens up a GUI that displays your program's stack, registers, variables, code, and assembly.  
It will create an instance of GDB in your shell, which you can use to modify the state of your program. 
When you run commands like `break`, `run`, `step`, and `next`, the GUI will update accordingly.

Any command line arguments given will be passed to GDB.

## Manual Installation

To create the output executable, clone the repository and `make` it. The executable will appear in the `build` folder.

These packages are requirements for the build process on Arch Linux:
  * base-devel (build-essential on Debian-based distros)
  * wxgtk (libwxgtk3.0-dev on Debian-based distros)

For any other distribution, you will have to find their equivalents on your respective package manager.
