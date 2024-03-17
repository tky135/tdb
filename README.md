# tdb
1. use fork to create two processes, the child process execute the file, the child process lets the parent process monitor it `ptrace(PTRACE_ME, ...)`. Note that ptrace is provide by the system
2. the parent process waits for the child process's singal using `waitpid`. This is when the child process is ready 
3. then the parent process uses `linenoise` to keep a commandline prompt
4. whenever a user enters a command, the command is executed and logged. 
   - `continue`: continues execution by `ptrace(PT_CONTINUE)`
   - `break`: software break: modify code at given address to call and interrupt handler `int 3`. 

## Extension
1. Hardware breakpoint: On x86 you can only have four hardware breakpoints set at a given time, but they give you the power to make them fire on reading from or writing to a given address rather than only executing code there.


## TDBG: A Linux C/C++ Debugger
Implemented from scratch a C/C++ debugger using C++. 
Use linenoise for handling commandline input, and libelfin for parsing the debug information. 
