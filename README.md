# tdbg
1. use fork to create two processes, the child process execute the file, the child process lets the parent process monitor it `ptrace(PTRACE_ME, ...)`. Note that ptrace is provide by the system
2. the parent process waits for the child process's singal using `waitpid`. This is when the child process is ready 
3. then the parent process uses `linenoise` to keep a commandline prompt
4. whenever a user enters a command, the command is executed and logged. 
   - `continue`: continues execution by `ptrace(PT_CONTINUE)`
   - `break`: software break: modify code at given address to call and interrupt handler `int 3`. 
   > To keep the address constant at every execution, we must disable ASLR by using the `personaly` function. 
   > To read the run-time mapping cat ` /proc/<pid>/maps`. 
   > To look at the relative code position, disassemble by: `objdump -d <your program>`. 
5. Allow user to view and modify registers: Need a data structure to store registers returned by ptrace. 
6. Allow user to read and write memory via ptrace PEEK and POKE. 
   > In a software breakpoint, we overwrite an instruction. To continue from breakpoint we must write back the instruction and execute from that instruction. Therefore, restore instruction -> pc = pc - 1 -> take one step -> overwrite instruction again(if it is executed again we want break(or not?)) -> continue execution
   > In x86_64 rip is PC. 
## Extension
1. Hardware breakpoint: On x86 you can only have four hardware breakpoints set at a given time, but they give you the power to make them fire on reading from or writing to a given address rather than only executing code there.
2. x86_64  has floating point and vector registers available
3. x86_64 also allows you to access some 64 bit registers as 32, 16, or 8 bit registers, but I’ll be sticking to 64
4. You might want to add support for reading and writing more than a word at a time, which you can do by incrementing the address each time you want to read another word. You could also use process_vm_readv and process_vm_writev or /proc/<pid>/mem instead of ptrace if you like.
## TDBG: A Linux C/C++ Debugger
Implemented from scratch a C/C++ debugger using C++. 
Use linenoise for handling commandline input, and libelfin for parsing the debug information. 
Allow user to view and modify arbitrary registers & memories. 
