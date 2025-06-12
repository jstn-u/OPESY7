#include "Process.h"
#include <iostream>

Process::Process(const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status)
    : name(name), currentLine(currentLine), totalLines(totalLines), timestamp(timestamp), status(status) {
    this->pid = 0; // Default PID, can be set later
    this->cpuCoreID = -1; // Default CPU core ID, can be set later
}

void Process::executeCurrentCommand(){
    if (currentLine < totalLines) {
        commandList[currentLine]->execute();
    }
    else {
        std::cout << "Error: currentLineOfInstruction exceeds commandList size." << std::endl;
    }
}

void Process::moveCurrentLine(){
    this->currentLine++;
}