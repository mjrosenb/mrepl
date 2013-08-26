mrepl is a Machine REPL.  The idea is that you can type in assembly, and see how
the state of the machine changes in response to every instruction.
ncursesw and readline are required.  an assembler called 'as', and a linker
called 'ld' should be in your path, but if your assembler/linker generate
different errors/warnings from mine, you may end up with a lack of feedback
when they throw errors.
Currently x64 is the only platform that is
supported, but it is pretty trivial to add new platforms in.

If you can build it at all, you can build it by simply typing make.
You can run it from the directory where it is built.

Try it out by typing in some instructions on your current architecture:

incl %eax
decq %rdx

etc.  Confusingly, arrow keys and ^P and ^N do two different things.  ^P and ^N
allow you to modify instructions that were entered in the past, while the arrow
keys should allow you to step through the execution of the code entered thusfar
^K while on a line should remove that line from the code (It doesn't get saved
anywhere, so be careful!) Hitting enter has a few different behaviors. If you
have selected a previously entered line, and have edited it, hitting enter will
save those changes back and nothing else (unfortunately, readline clears the
current input line, and that messes up some later stuff). If you haven't edited
the line, then hitting enter will insert a new blank line after the current
one. Since lines are always inserted *after*, this means that currently, there
isn't a good way to insert a line before the first line.
