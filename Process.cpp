#include "Process.h"
#include "PrintCommand.h"
#include <iostream>
#include <random>
#include <ctime>
#include <regex>
#include <algorithm>
#include <cctype>

static std::mt19937 rng;

Process::Process(int pid, const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status, int memSize)
    : pid(pid), name(name), currentLine(currentLine), totalLines(totalLines), timestamp(timestamp), status(status), memSize(memSize) {
    this->cpuId = -1;
    // Initialize all 32 variables
    for (int i = 1; i <= 32; ++i) {
        std::string var = "var" + std::to_string(i);
        variables[i] = {0, var, false};
    }
}

Process::Process(int pid, std::string processName, int memSize)
    : pid(pid), name(processName), currentLine(0), totalLines(0), timestamp(""), status(""), cpuId(-1), startTime(0), memSize(memSize) {
    // Initialize all 32 variables
    for (int i = 1; i <= 32; ++i) {
        std::string var = "var" + std::to_string(i);
        variables[i] = {0, var, false};
    }
}

Process::Process() 
    : pid(0), name(""), currentLine(0), totalLines(0), timestamp(""), status(""), cpuId(-1), startTime(0), memSize(0) {
    // Initialize all 32 variables
    for (int i = 1; i <= 32; ++i) {
        std::string var = "var" + std::to_string(i);
        variables[i] = {0, var, false};
    }
}

void Process::moveCurrentLine(){
    this->currentLine++;
}

std::time_t Process::getStartTime() const {
    return startTime;
}

int Process::getUsedMemory() {
    int totalUsed = 0;
    for (const std::string& cmd : this->getAllLogs()) {
        totalUsed += this->getInstructionSize(cmd);
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
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

std::string ltrim(const std::string& s);

int Process::getInstructionSize(const std::string& instr) {
    std::string s = ltrim(instr); // Use trimmed string for checks
    if (s.find("PRINT") == 0) return 1;
    if (s.find("DECLARE") == 0) return 2;
    if (s.find("ADD") == 0 || s.find("SUBTRACT") == 0) return 3;
    if (s.find("SLEEP") == 0) return 1;
    if (s.find("FOR") == 0) {
        // Parse FOR([body], count) with nested brackets
        size_t bodyStart = s.find("[") + 1;
        int bracketCount = 1;
        size_t i = bodyStart;
        while (i < s.size() && bracketCount > 0) {
            if (s[i] == '[') bracketCount++;
            else if (s[i] == ']') bracketCount--;
            i++;
        }
        size_t bodyEnd = i - 1;
        std::string body = s.substr(bodyStart, bodyEnd - bodyStart);
        size_t countStart = s.find(",", bodyEnd) + 1;
        size_t countEnd = s.find(")", countStart);
        int count = std::stoi(s.substr(countStart, countEnd - countStart));
        int bodySize = getInstructionSize(body);
        return 1 + bodySize * count;
    }
    if (s.find("READ") == 0) return 3;
    if (s.find("WRITE") == 0) return 2;
    return 1;
}

// Helper to generate a random instruction string, possibly a nested FOR
std::string Process::generateRandomInstruction(int nestingLevel, int& instrBytes, int maxNesting) {
    static const std::vector<std::string> instrTypes = {
        "PRINT", "DECLARE", "ADD", "SUBTRACT", "SLEEP", "FOR", "READ", "WRITE"
    };

    std::string type = instrTypes[getRandomInt(0, instrTypes.size() - 1)];
    std::string msg;
    bool skip = false;

    if (type == "FOR" && nestingLevel < maxNesting) {
        int forCount = getRandomInt(1, 3);
        int bodyBytes = 0;
        std::string bodyInstr = generateRandomInstruction(nestingLevel + 1, bodyBytes, maxNesting);
        msg = "FOR([" + bodyInstr + "], " + std::to_string(forCount) + ")";
        instrBytes = 1 + bodyBytes * forCount;
        return msg;
    }

    if (type == "PRINT") {
        msg = "PRINT(\"Hello world from process!\")";
        instrBytes = 1;
    } else if (type == "DECLARE") {
        // Find the first undeclared variable in ascending order
        int nextIndex = -1;
        for (int i = 1; i <= 32; ++i) {
            if (!variables[i].declared) {
                nextIndex = i;
                break;
            }
        }
        if (nextIndex == -1) {
            instrBytes = 0;
            return ""; // All variables already declared, skip
        }
        std::string var = "var" + std::to_string(nextIndex);
        uint16_t value = getRandomInt(0, 65535);
        variables[nextIndex].value = value;
        variables[nextIndex].declared = true; // Mark as declared
        msg = "DECLARE(" + var + ", " + std::to_string(value) + ")";
        instrBytes = 2;
    } else if (type == "ADD" || type == "SUBTRACT") {
        int targetIdx = getRandomInt(1, 32);
        int src1Idx = getRandomInt(1, 32);
        int src2Idx = getRandomInt(1, 32);
        std::string target = "var" + std::to_string(targetIdx);
        std::string src1 = (getRandomInt(0, 1) == 0)
            ? std::to_string(getRandomInt(0, 100))
            : "var" + std::to_string(src1Idx);
        std::string src2 = (getRandomInt(0, 1) == 0)
            ? std::to_string(getRandomInt(0, 100))
            : "var" + std::to_string(src2Idx);
        // Mark all variables used as declared (with value 0 if not already)
        variables[targetIdx].declared = true;
        variables[src1Idx].declared = true;
        variables[src2Idx].declared = true;
        msg = type + "(" + target + ", " + src1 + ", " + src2 + ")";
        instrBytes = 3;
    } else if (type == "SLEEP") {
        int ticks = getRandomInt(1, 50);
        msg = "SLEEP(" + std::to_string(ticks) + ")";
        instrBytes = 1;
    } else if (type == "READ") {
        // Collect declared variables
        std::vector<std::string> usableVars;
        for (const auto& [idx, v] : variables) {
            if (v.declared) usableVars.push_back(v.varname);
        }
        if (usableVars.empty()) {
            instrBytes = 0;
            return ""; // Skip generating this instruction
        }
        int varIdx = getRandomInt(0, usableVars.size() - 1);
        std::string var = usableVars[varIdx];

        int baseAddr = 0x0040;
        int maxEvenAddr = baseAddr + memSize - 2; // Exclude the last even address
        int addr = baseAddr;
        if (maxEvenAddr >= baseAddr) {
            int numEvenAddrs = ((maxEvenAddr - baseAddr) / 2) + 1;
            int offset = getRandomInt(0, numEvenAddrs - 1) * 2;
            addr = baseAddr + offset;
        }
        std::stringstream ss;
        ss << "0x" << std::hex << addr;
        msg = "READ " + var + " " + ss.str();
        instrBytes = 3;
    } else if (type == "WRITE") {
        int baseAddr = 0x0040;
        int maxEvenAddr = baseAddr + memSize - 2; // Exclude the last even address
        int addr = baseAddr;
        if (maxEvenAddr >= baseAddr) {
            int numEvenAddrs = ((maxEvenAddr - baseAddr) / 2) + 1;
            int offset = getRandomInt(0, numEvenAddrs - 1) * 2;
            addr = baseAddr + offset;
        }
        int value = getRandomInt(0, 65535);
        std::stringstream ss;
        ss << "0x" << std::hex << addr;
        msg = "WRITE " + ss.str() + " " + std::to_string(value);
        instrBytes = 2;
    }

    return msg;
}

void Process::createPrintCommands(int totalIns) {
    declaredVars.clear(); // <-- Add this line at the start
    if (name.find("auto_proc_") != 0) {
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

    int i = 0;
    int var_count = 0;
    while (i < totalIns) {
        int instrBytes = 0;
        std::string msg = generateRandomInstruction(0, instrBytes, 3);
        commands.push_back(new PrintCommand(msg));
        i++; // Always increment by 1 instruction
    }
}

void Process::executeCurrentCommand(int cpuId, std::string processName, std::string time) {
    if (currentLine < commands.size()) {
        commands[currentLine]->execute(cpuId, processName, std::time(nullptr));
    }
}

int Process::getEndAddress() const{
    int startAddress = 0x0040;
    int endAddress = startAddress + memSize - 1; // memSize is mem-per-proc (bytes)
    return endAddress;
}

// setEndTime, getEndTime, getEndTimeString are now implemented inline in the header, so no additional implementation needed here.

// Helper to trim leading spaces
std::string ltrim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : s.substr(start);
}
