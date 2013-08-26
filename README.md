mrepl is a Machine REPL.  The idea is that you can type in assembly, and see how
the state of the machine changes in response to every instruction.
ncursesw and readline are required.  Currently x64 is the only platform that is
supported, but it is pretty trivial to add new platforms in.

If you can build it at all, you can build it by simply typing make.
You can run it from the directory where it is built.  