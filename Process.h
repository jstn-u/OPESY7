#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include "PrintCommand.h"

class Process{
enum ProcessState{
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

private:
    int pid;
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;
    std::string status;
    int cpuId;
    std::time_t startTime;
    std::vector<PrintCommand*> commands;

public:
    Process() = default;
    Process(const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status)
    : name(name), currentLine(currentLine), totalLines(totalLines), timestamp(timestamp), status(status) {}
    Process(int pid, std::string processName);
    ~Process() = default;

    int getPid() const { return pid; }
    const std::string& getName() const { return name; }
    int getCurrentLine() const { return currentLine; }
    int getTotalLines() const { return totalLines; }
    const std::string& getTimestamp() const { return timestamp; }
    const std::string& getStatus() const { return status; }

    void setStatus(const std::string& newStatus) { status = newStatus; };
    void moveCurrentLine();
    void executeCurrentCommand(int cpuId, std::string processName, std::string time);
    void create100PrintCommands();
    std::time_t getStartTime() const;
};