#include "Process.h"
#include "PrintCommand.h"
#include <iostream>
#include <random>
#include <ctime>

Process::Process(int pid, const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status, int memSize)
    : pid(pid), name(name), currentLine(currentLine), totalLines(totalLines), timestamp(timestamp), status(status), memSize(memSize) {
    this->cpuId = -1; // Default CPU core ID, can be set later
}

Process::Process(int pid, std::string processName, int memSize)
    : pid(pid), name(processName), currentLine(0), totalLines(0), timestamp(""), status(""), cpuId(-1), startTime(0), memSize(memSize) {}

Process::Process() 
    : pid(0), name(""), currentLine(0), totalLines(0), timestamp(""), status(""), cpuId(-1), startTime(0), memSize(0) {}

void Process::moveCurrentLine(){
    this->currentLine++;
}

std::time_t Process::getStartTime() const {
    return startTime;
}

int Process::getUsedMemory() const {
    int totalUsed = 0;
    for (const std::string& cmd : this->getAllLogs()) {
        totalUsed += getInstructionSize(cmd);
    }
    return totalUsed;
}

//was edited out 
/*
void Process::createPrintCommands(int totalIns) {
    for (int i = 0; i < totalIns; ++i) {
        std::string msg = "Hello world from " + name + "!";
        commands.push_back(new PrintCommand(msg));
    }
}*/

int getRandomInt(int min, int max) {
    static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

int getInstructionSize(const std::string& instr) {
    if (instr.find("PRINT") == 0) return 1;
    if (instr.find("DECLARE") == 0) return 2;
    if (instr.find("ADD") == 0 || instr.find("SUBTRACT") == 0) return 3;
    if (instr.find("SLEEP") == 0) return 1;
    if (instr.find("FOR") == 0) return 3;
    if (instr.find("READ") == 0) return 3;
    if (instr.find("WRITE") == 0) return 2;
    return 1;
}

void Process::createPrintCommands(int totalIns) {
    // If this process is manually added (screen -s <process_name>), use alternating PRINT/ADD logic
    // We'll assume that if the process name does not start with "auto_proc_", it's a manual process

    // TODO : Change the code so that all instructions related to reading will search the variable table
    // to check if the variable has been declared. If the variable does not exist
    // (no variable has been declared in that memory address), it will return 0. Attempting to read
    // from an invalid memory address will throw an error.

    if (name.find("auto_proc_") != 0) {
        variables["x"] = 0;
        int xVal = 0;
        for (int i = 0; i < totalIns; ++i) {
            std::string msg;
            if (i % 2 == 0) {
                msg = "PRINT(\"Value from: " + std::to_string(xVal) + ")";
            } else {
                int addVal = getRandomInt(1, 10);
                msg = "ADD(x, x, " + std::to_string(addVal) + ")";
                xVal += addVal;
            }
            commands.push_back(new PrintCommand(msg));
        }
        return;
    }
    static const std::vector<std::string> instrTypes = {
        "PRINT", "DECLARE", "ADD", "SUBTRACT", "SLEEP", "FOR", "READ", "WRITE"
    };

    int i = 0;
    while (i < totalIns) {
        std::string type = instrTypes[getRandomInt(0, instrTypes.size() - 1)];
        std::string msg;

        // 3 bytes (tentative)
        if (type == "FOR" && i + 3 <= totalIns) {
            int forCount = getRandomInt(1, 3);
            for (int j = 0; j < forCount && i < totalIns; ++j) {
                msg = "FOR([PRINT(\"Hello world\")], " + std::to_string(forCount) + ")";
                commands.push_back(new PrintCommand(msg));
                ++i;
            }
            continue;
        }

        // 1 byte
        if (type == "PRINT") {
            msg = "PRINT(\"Hello world from " + name + "!\")";
        }

        // 2 byte
        else if (type == "DECLARE") {
            std::string var = "var" + std::to_string(getRandomInt(1, 32));
            int value = getRandomInt(0, 65535);
            msg = "DECLARE(" + var + ", " + std::to_string(value) + ")";
        }

        // 3 byte
        else if (type == "ADD" || type == "SUBTRACT") {
            std::string target = "var" + std::to_string(getRandomInt(1, 32));
            std::string src1 = (getRandomInt(0, 1) == 0)
                ? std::to_string(getRandomInt(0, 100))
                : "var" + std::to_string(getRandomInt(1, 32));
            std::string src2 = (getRandomInt(0, 1) == 0)
                ? std::to_string(getRandomInt(0, 100))
                : "var" + std::to_string(getRandomInt(1, 32));
            msg = type + "(" + target + ", " + src1 + ", " + src2 + ")";
        }

        // 1 byte
        else if (type == "SLEEP") {
            int ticks = getRandomInt(1, 50);
            msg = "SLEEP(" + std::to_string(ticks) + ")";
        }

        // 3 bytes
        else if (type == "READ") {
            std::string var = "var" + std::to_string(getRandomInt(1, 32));
            // Generate a random 4-digit hex address in range 0x1000-0x1FFF
            int addr = 0x1000 + getRandomInt(0, 0x0FFF);
            std::stringstream ss;
            ss << "0x" << std::hex << addr;
            msg = "READ " + var + " " + ss.str();
        }

        // 2 bytes
        else if (type == "WRITE") {
            // Generate a random 4-digit hex address in range 0x1000-0x1FFF
            int addr = 0x1000 + getRandomInt(0, 0x0FFF);
            int value = getRandomInt(0, 65535);
            std::stringstream ss;
            ss << "0x" << std::hex << addr;
            msg = "WRITE " + ss.str() + " " + std::to_string(value);
        }

        commands.push_back(new PrintCommand(msg));
        ++i;
    }
}

void Process::executeCurrentCommand(int cpuId, std::string processName, std::string time) {
    if (currentLine < commands.size()) {
        commands[currentLine]->execute(cpuId, processName, std::time(nullptr));
    }
}

// setEndTime, getEndTime, getEndTimeString are now implemented inline in the header, so no additional implementation needed here.
