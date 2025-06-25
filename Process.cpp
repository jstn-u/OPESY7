#include "Process.h"
#include "PrintCommand.h"
#include <iostream>

Process::Process(int pid, const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status)
    : pid(pid), name(name), currentLine(currentLine), totalLines(totalLines), timestamp(timestamp), status(status) {
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

void Process::createPrintCommands(int totalIns) {
    for (int i = 0; i < totalIns; ++i) {
        std::string msg = "Hello world from " + name + "!";
        commands.push_back(new PrintCommand(msg));
    }
}

void Process::executeCurrentCommand(int cpuId, std::string processName, std::string time) {
    if (currentLine < commands.size()) {
        commands[currentLine]->execute(cpuId, processName, std::time(nullptr));
    }
}

// setEndTime, getEndTime, getEndTimeString are now implemented inline in the header, so no additional implementation needed here.