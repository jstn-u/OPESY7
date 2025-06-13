#pragma once
#include <string>
#include <vector>
#include <memory>
#include "ICommand.h"

class Process{
struct requirementFlags{
    bool requiredFiles;
    int numFiles;
    bool requireMemory;
    int memoryRequired;
};

enum ProcessState{
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

private:
    std::vector<std::shared_ptr<ICommand>> commandList;
    int pid;
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;
    std::string status;
    int cpuCoreID; // which core it is assigned to

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
    ICommand* getCommand(int idx) const { return commandList[idx].get(); }
    void addCommand(ICommand* cmd);

    void setStatus(const std::string& newStatus) { status = newStatus; };
    void moveCurrentLine();
    void executeCurrentCommand();
};