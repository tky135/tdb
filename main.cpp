#include <iostream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <iomanip>
#include <sys/user.h>
extern "C" {
    #include "linenoise.h"
}

enum class reg {
    rax, rbx, rcx, rdx,
    rdi, rsi, rbp, rsp,
    r8,  r9,  r10, r11,
    r12, r13, r14, r15,
    rip, rflags,    cs,
    orig_rax, fs_base,
    gs_base,
    fs, gs, ss, ds, es
};
class breakpoint {
    public:
        breakpoint(pid_t pid, std::intptr_t addr)
            : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_word{} {}
        breakpoint() {}
        void enable();
        void disable();
        bool is_enabled() { return m_enabled; }
        uint64_t get_saved_word() { return m_saved_word; }
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
        unsigned long long read_reg(std::string reg_name);
        unsigned long long& reg_switch_on_name(std::string reg_name, user_regs_struct& regs_struct);
        void write_reg(std::string reg_name, unsigned long long value);
        void dump_registers();
        unsigned long long read_memory(std::intptr_t address);
        void write_memory(std::intptr_t address, unsigned long long value);
        void step_over_breakpoint();
    private:
        std::string m_prog_name;
        pid_t m_pid;
        std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;
        user_regs_struct temp_regs;
        std::map<std::string, reg> get_reg_enum {
        {"rax", reg::rax}, {"rbx", reg::rbx}, {"rcx", reg::rcx}, {"rdx", reg::rdx},
        {"rdi", reg::rdi}, {"rsi", reg::rsi}, {"rbp", reg::rbp}, {"rsp", reg::rsp},
        {"r8", reg::r8}, {"r9", reg::r9}, {"r10", reg::r10}, {"r11", reg::r11},
        {"r12", reg::r12}, {"r13", reg::r13}, {"r14", reg::r14}, {"r15", reg::r15},
        {"rip", reg::rip}, {"rflags", reg::rflags}, {"cs", reg::cs},
        {"orig_rax", reg::orig_rax}, {"fs_base", reg::fs_base}, {"gs_base", reg::gs_base},
        {"fs", reg::fs}, {"gs", reg::gs}, {"ss", reg::ss}, {"ds", reg::ds}, {"es", reg::es}
    };
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
    } else if (is_prefix(command, "register")) {
        if (is_prefix(args[1], "dump")) {
            dump_registers();
        } else if (is_prefix(args[1], "read")) {
            std::string reg_name = args[2];
            // check name
            auto it = get_reg_enum.find(reg_name);
            if (it == get_reg_enum.end()) {
                std::cerr << "Unknown register: " << reg_name << std::endl;
                return;
            }
            std::cout << reg_name << ":\t0x" << std::setfill('0') << std::setw(16) << std::hex << read_reg(reg_name) << std::endl;
        } else if (is_prefix(args[1], "write")) {
            std::string reg_name = args[2];
            // check name
            auto it = get_reg_enum.find(reg_name);
            if (it == get_reg_enum.end()) {
                std::cerr << "Unknown register: " << reg_name << std::endl;
                return;
            }
            unsigned long long value = std::stoull(args[3].substr(2), 0, 16);
            write_reg(reg_name, value);
        } else {
            std::cerr << "not implemented" << std::endl;
        }
    } else if (is_prefix(command, "memory")) {
        if (is_prefix(args[1], "read")) {
            std::intptr_t address = std::stol(args[2].substr(2), 0, 16);
            std::cout << "0x" << std::setfill('0') << std::setw(16) << std::hex << read_memory(address) << std::endl;
        } else if (is_prefix(args[1], "write")) {
            std::intptr_t address = std::stol(args[2].substr(2), 0, 16);
            unsigned long long value = std::stoull(args[3].substr(2), 0, 16);
            write_memory(address, value);
        } else {
            std::cerr << "not implemented" << std::endl;
        }
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
    step_over_breakpoint();
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    int wait_status; 
    waitpid(m_pid, &wait_status, 0);

    // if a breakpoint is hit, tell the user
    auto possible_breakpoint_location = read_reg("rip") - 1;
    if (m_breakpoints.count(possible_breakpoint_location)) {
        std::cout << "Hit a breakpoint at: 0x" << std::hex << possible_breakpoint_location << std::endl;
    }
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
unsigned long long & debugger::reg_switch_on_name(std::string reg_name, user_regs_struct& regs_struct) {
    // reg_name must be checked before giving to this function
    reg reg_enum = get_reg_enum[reg_name];
    switch (reg_enum) {
        case reg::rax: return regs_struct.rax;
        case reg::rbx: return regs_struct.rbx;
        case reg::rcx: return regs_struct.rcx;
        case reg::rdx: return regs_struct.rdx;
        case reg::rdi: return regs_struct.rdi;
        case reg::rsi: return regs_struct.rsi;
        case reg::rbp: return regs_struct.rbp;
        case reg::rsp: return regs_struct.rsp;
        case reg::r8:  return regs_struct.r8;
        case reg::r9:  return regs_struct.r9;
        case reg::r10: return regs_struct.r10;
        case reg::r11: return regs_struct.r11;
        case reg::r12: return regs_struct.r12;
        case reg::r13: return regs_struct.r13;
        case reg::r14: return regs_struct.r14;
        case reg::r15: return regs_struct.r15;
        case reg::rip: return regs_struct.rip;
        case reg::rflags: return regs_struct.eflags;
        case reg::cs: return regs_struct.cs;
        case reg::orig_rax: return regs_struct.orig_rax;
        case reg::fs_base: return regs_struct.fs_base;
        case reg::gs_base: return regs_struct.gs_base;
        case reg::fs: return regs_struct.fs;
        case reg::gs: return regs_struct.gs;
        case reg::ss: return regs_struct.ss;
        case reg::ds: return regs_struct.ds;
        case reg::es: return regs_struct.es;
        default:
            throw std::runtime_error("Unsupported register: " + reg_name);
    }
}
unsigned long long debugger::read_reg(std::string reg_name) {
    user_regs_struct reg_struct;
    ptrace(PT_GETREGS, m_pid, nullptr, &reg_struct);
    return reg_switch_on_name(reg_name, reg_struct);
}

void debugger::write_reg(std::string reg_name, unsigned long long value) {
    user_regs_struct reg_struct;
    ptrace(PT_GETREGS, m_pid, nullptr, &reg_struct);
    reg_switch_on_name(reg_name, reg_struct) = value;
    ptrace(PT_SETREGS, m_pid, nullptr, &reg_struct);
}

void debugger::dump_registers() {
    for (const auto& kv: get_reg_enum) {
        std::cout << kv.first << ":\t0x" << std::setfill('0') << std::setw(16) << std::hex << read_reg(kv.first) << std::endl;
    }
}

unsigned long long debugger::read_memory(std::intptr_t address) {
    return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void debugger::write_memory(std::intptr_t address, unsigned long long value) {
    ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

void debugger::step_over_breakpoint() {
    // executed after a SIGTRAP signal is received
    // may be a breakpoint
    // check if rip - 1 is a breakpoint (rip is PC, except that rip is limited to x86_64)
    auto possible_breakpoint_location = read_reg("rip") - 1;
    if (m_breakpoints.count(possible_breakpoint_location)) {
        std::cout << "\nresuming from breakpoint: " << possible_breakpoint_location << std::endl;
        breakpoint& bp = m_breakpoints[possible_breakpoint_location];
        if (bp.is_enabled()) {
            // disable breakpoint to restore original instruction
            bp.disable();
            // decrement rip by 1 to re-execute the instruction
            write_reg("rip", possible_breakpoint_location);
            ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
            int wait_status;
            waitpid(m_pid, &wait_status, 0);
            // re-enable the breakpoint
            bp.enable();
            
        }
    }
}   