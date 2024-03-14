#include <iostream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <string>
#include <sstream>
#include <vector>
extern "C" {
    #include "linenoise.h"
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
    private:
        std::string m_prog_name;
        pid_t m_pid;
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
    std::cout << "Handling command: " << line << std::endl;
    std::vector<std::string> args = split(line, ' ');
    std::cout << "Args size: " << args.size() << std::endl;
    std::string command = args[0];
    std::cout << "Command: $" << command << "$" << std::endl;
    std::cout << command.size() << std::endl;
    if (command.size() == 0) return;
    else if (is_prefix(command, "continue")) {
        // continue_execution();
        continue_execution();
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
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Program name not specified";
        return -1;
    }

    char* prog = argv[1];
    pid_t pid = fork();
    if (pid == 0) {
        // child process
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

