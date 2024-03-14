# tdb
1. use fork to create two processes, the child process execute the file, the child process lets the parent process monitor it `ptrace(PTRACE_ME, ...)`. Note that ptrace is provide by the system
2. the parent process waits for the child process's singal using `waitpid`. This is when the child process is ready 
3. then the parent process uses `linenoise` to keep a commandline prompt
4. whenever a user enters a command, the command is executed and logged. 
   - `continue`: continues execution by `ptrace(PT_CONTINUE)`