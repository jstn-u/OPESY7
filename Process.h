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
    std::string endTime;
    std::vector<PrintCommand*> commands;

public:
    void setCpuId(int id) { cpuId = id; }
    int getCpuId() const { return cpuId; }
    void addCommand(PrintCommand* cmd) { commands.push_back(cmd); }
    void create100PrintCommands();
    void executeCurrentCommand(int cpuId, std::string processName, std::string time);

    Process();
    Process(const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status);
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
    std::time_t getStartTime() const;
    void setEndTime(std::string t) { endTime = t; };
    std::string getEndTime() const { return endTime; };
    std::string getEndTimeString();
};