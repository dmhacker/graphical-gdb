# gg

*It's as simple as*

```
gg a.out
```

## Summary

**gg** (graphical gdb) is a gdb wrapper that provides you with a helpful, pretty view of your program's current state. 

Running gg opens up a GUI that displays your program's stack, registers, variables, code, and assembly.  gg will also create an instance of gdb in your shell, which you can use to modify the state of your program. When you run commands like `break`, `run`, `step`, and `next`, the gg GUI will update accordingly.

Any arguments that you pass to gg on startup will be passed to gdb.

## Installation

```
sudo apt-get install gg
```

Note that gg requires g++ for compilation purposes.

## Backstory

gg was intended for users who are running gdb on a remote machine. Specifically, it was designed for UCSD's [CSE 30](https://cse.ucsd.edu/undergraduate/courses/course-descriptions/cse-30-computer-organization-and-systems-programming) class, where all programming assignments are done over SSH. As long as X11 forwarding is enabled for the connection, the GUI will display correctly on the host machine.
