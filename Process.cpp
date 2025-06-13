#include "Process.h"
#include <iostream>

Process::Process(const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status)
    : name(name), currentLine(currentLine), totalLines(totalLines), timestamp(timestamp), status(status) {
    this->pid = 0; // Default PID, can be set later
    this->cpuId = -1; // Default CPU core ID, can be set later
}

void Process::moveCurrentLine(){
    this->currentLine++;
}

void Process::create100PrintCommands()
{
}

std::time_t Process::getStartTime() const {
    return startTime;
}