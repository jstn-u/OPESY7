#include "Process.h"
#include "PrintCommand.h"
#include <iostream>

Process::Process(const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status)
    : name(name), currentLine(currentLine), totalLines(totalLines), timestamp(timestamp), status(status) {
    this->pid = 0; // Default PID, can be set later
    this->cpuId = -1; // Default CPU core ID, can be set later
}

Process::Process() 
    : pid(0), name(""), currentLine(0), totalLines(0), timestamp(""), status(""), cpuId(-1), startTime(0) {}

void Process::moveCurrentLine(){
    this->currentLine++;
}

std::time_t Process::getStartTime() const {
    return startTime;
}

void Process::create100PrintCommands() {
    for (int i = 0; i < 100; ++i) {
        std::string msg = "Hello world from " + name + "!";
        commands.push_back(new PrintCommand(msg));
    }
}

void Process::executeCurrentCommand(int cpuId, std::string processName, std::string time) {
    if (currentLine < commands.size()) {
        commands[currentLine]->execute(cpuId, processName, std::time(nullptr));
    }
}