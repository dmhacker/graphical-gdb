# graphical-gdb

*It's as simple as*

```
gg a.out
```

## Summary

**gg** (graphical gdb) is a wrapper around GDB that provides you with a visual of your program's current state. 

Running this program opens up a GUI that displays your program's stack, registers, variables, code, and assembly.  
It will create an instance of GDB in your shell, which you can use to modify the state of your program. 
When you run commands like `break`, `run`, `step`, and `next`, the GUI will update accordingly.

Any command line arguments given will be passed to GDB.

## Manual Installation

```
git clone https://github.com/dmhacker/graphical-gdb
cd graphical-gdb
make
```

The output executable is in the `build` folder.

These packages are requirements for the build process on Arch Linux:
  * base-devel (build-essential on Debian-based distros)
  * wxgtk (libwxgtk3.0-dev on Debian-based distros)

For any other distribution, you will have to find their equivalents on your respective package manager.

## Backstory

This program is intended for users who are running GDB remotely.
It was designed for use in UCSD's [CSE 30](https://cse.ucsd.edu/undergraduate/courses/course-descriptions/cse-30-computer-organization-and-systems-programming) class, where all programming assignments are done over SSH. 
As long as X11 forwarding is enabled for the connection, the GUI should display correctly on the host machine.
