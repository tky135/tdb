#include <iostream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
extern "C" {
    #include "linenoise.h"
}
class breakpoint {
    public:
        breakpoint(pid_t pid, std::intptr_t addr)
            : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_word{} {}
        breakpoint() {}
        void enable();
        void disable();
    private:
        pid_t m_pid;
        std::intptr_t m_addr;
        bool m_enabled;
        uint64_t m_saved_word;
};

void breakpoint::enable() {
    // save original byte
    m_saved_word = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);    // read a word; Linux does not have separate text and data address spaces

    // replace with 0xcc
    uint64_t data_with_int3 = (m_saved_word & ~0xff) | 0xcc;    // replace the least significant byte with 0xcc
    // when you read it as int, little endian -> smallest address is least significant
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);
    m_enabled = true;
}

void breakpoint::disable() {
    // restore saved bytes

    ptrace(PTRACE_POKEDATA, m_pid, m_addr, m_saved_word);
    m_enabled = false;
}
class debugger {
    public:
        debugger(std::string prog_name, pid_t pid)
            : m_prog_name{std::move(prog_name)}, m_pid{pid} {}
        void run();
        void handle_command(const std::string& line);
        std::vector<std::string> split(const std::string& s, char delim);
        bool is_prefix(const std::string& s, const std::string& of);
        void continue_execution();
        void set_breakpoint_at_address(std::intptr_t address);
    private:
        std::string m_prog_name;
        pid_t m_pid;
        std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;
};

void debugger::run() {
    int wait_status;
    waitpid(m_pid, &wait_status, 0);    // wait for the SIGTRAP signal sent by the child process to the parent process. In the context of ptrace, this signal is designed to be sent when the child process entries/exits a system call

    char* line = nullptr;
    while ((line = linenoise("tdbg> ")) != nullptr) {   // will loop forever? 
        handle_command(line);   // implicit type conversion will create an rvalue, the the argument must be const lvalue reference or rvalue reference
        linenoiseHistoryAdd(line);
        linenoiseFree(line);    // almost just free so linenoise returns a pointer to the heap 
    }
}

void debugger::handle_command(const std::string& line) {
    std::vector<std::string> args = split(line, ' ');
    std::string command = args[0];
    if (command.size() == 0) return;
    else if (is_prefix(command, "continue")) {
        // continue_execution();
        continue_execution();
    } else if (is_prefix(command, "break")) {
        // TODO: error checking
        std::intptr_t address = std::stol(args[1].substr(2), 0, 16);  // assuming user entered 0x
        set_breakpoint_at_address(address);
    } else {
        std::cerr << "not implemented" << std::endl;
    }

}

std::vector<std::string> debugger::split(const std::string& s, char delim) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = s.find(delim);
    while (end != std::string::npos) {
        result.push_back(s.substr(start, end - start));
        start = end + 1;
        end = s.find(delim, start);
    }
    result.push_back(s.substr(start, s.size() - start));
    return result;

}

bool debugger::is_prefix(const std::string& prefix, const std::string& longstring) {
    if (prefix.size() > longstring.size()) {
        return false;
    } else {
        return std::equal(prefix.begin(), prefix.end(), longstring.begin());    // this is almost is_prefix
    }
}

void debugger::continue_execution() {
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    int wait_status; 
    waitpid(m_pid, &wait_status, 0);
}

void debugger::set_breakpoint_at_address(std::intptr_t address) {
    std::cout << "Adding breakpoint at address: 0x" << std::hex << address << std::endl;
    breakpoint bp {m_pid, address};
    bp.enable();
    m_breakpoints[address] = bp;    // copy is cheap    // the left-hand side must evaluate to something. Therefore a default constrcutor is needed
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Program name not specified";
        return -1;
    }

    char* prog = argv[1];
    pid_t pid = fork();
    if (pid == 0) {
        // child process
        personality(ADDR_NO_RANDOMIZE);
        std::cout << "Child process " << std::endl;

        // replace the current process with the executable
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);    // child declares it's being traced by the parent. Other parameters are ommitted
        execl(prog, prog, nullptr); // the list of arguments are terminated by null. TODO: support execution of programs with arguments
    } else if (pid > 0){
        // parent process
        std::cout << "Started to debug process " << pid << std::endl;
        debugger dbg {prog, pid};
        dbg.run();
    }
}

