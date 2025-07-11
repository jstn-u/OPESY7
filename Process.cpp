#include "Process.h"
#include "PrintCommand.h"
#include <iostream>
#include <random>
#include <ctime>

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

void Process::createPrintCommands(int totalIns) {
    // If this process is manually added (screen -s <process_name>), use alternating PRINT/ADD logic
    // We'll assume that if the process name does not start with "auto_proc_", it's a manual process
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
        "PRINT", "DECLARE", "ADD", "SUBTRACT", "SLEEP", "FOR"
    };

    int i = 0;
    while (i < totalIns) {
        std::string type = instrTypes[getRandomInt(0, instrTypes.size() - 1)];
        std::string msg;

        if (type == "FOR" && i + 3 <= totalIns) {
            int forCount = getRandomInt(1, 3);
            for (int j = 0; j < forCount && i < totalIns; ++j) {
                msg = "FOR([PRINT(\"Hello world\")], " + std::to_string(forCount) + ")";
                commands.push_back(new PrintCommand(msg));
                ++i;
            }
            continue;
        }

        if (type == "PRINT") {
            msg = "PRINT(\"Hello world from " + name + "!\")";
        }
        else if (type == "DECLARE") {
            std::string var = "var" + std::to_string(getRandomInt(1, 5));
            int value = getRandomInt(0, 65535);
            msg = "DECLARE(" + var + ", " + std::to_string(value) + ")";
        }
        else if (type == "ADD" || type == "SUBTRACT") {
            std::string target = "var" + std::to_string(getRandomInt(1, 5));
            std::string src1 = (getRandomInt(0, 1) == 0)
                ? std::to_string(getRandomInt(0, 100))
                : "var" + std::to_string(getRandomInt(1, 5));
            std::string src2 = (getRandomInt(0, 1) == 0)
                ? std::to_string(getRandomInt(0, 100))
                : "var" + std::to_string(getRandomInt(1, 5));
            msg = type + "(" + target + ", " + src1 + ", " + src2 + ")";
        }
        else if (type == "SLEEP") {
            int ticks = getRandomInt(1, 50);
            msg = "SLEEP(" + std::to_string(ticks) + ")";
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
