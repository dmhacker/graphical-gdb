# graphical-gdb

*It's as simple as*

```
gg a.out
```

## Summary

**gg** (graphical gdb) is a gdb wrapper that provides you with a helpful, pretty view of your program's current state. 

Running gg opens up a GUI that displays your program's stack, registers, variables, code, and assembly.  gg will also create an instance of gdb in your shell, which you can use to modify the state of your program. When you run commands like `break`, `run`, `step`, and `next`, the gg GUI will update accordingly.

Any arguments that you pass to gg on startup will be passed to gdb.

## Automatic Installation 

Coming soon!

## Manual Installation

Alternatively, you could clone this repository and build directly from source.<br>
These packages are requirements for the build process on Arch Linux:
  * base-devel (build-essential on Debian-based distros)
  * wxgtk (libwxgtk3.0-dev on Debian-based distros)

For any other distribution, you will have to find their equivalents on your respective package
manager.

After you have all the packages and you've `cd`'d into a cloned version of this repository, run
```
make
```
The `gg` executable will be in the build/ folder.

## Backstory

gg was intended for users who are running gdb on a remote machine. Specifically, I designed it for UCSD's [CSE 30](https://cse.ucsd.edu/undergraduate/courses/course-descriptions/cse-30-computer-organization-and-systems-programming) class, where all programming assignments are done over SSH. As long as X11 forwarding is enabled for the connection, the GUI will display correctly on the host machine.
