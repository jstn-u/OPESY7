#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <map>
#include <variant>
#include <functional>
#include <mutex>
#include <unordered_map>
#include "PrintCommand.h"

struct var_map{
    uint16_t value;
    std::string varname;
};

class Instruction;

class Process{
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
    std::map<int, var_map> variables; // symbol table for variables
    std::vector<Instruction*> instructions;
    int instructionPointer = 0;
    int sleepTicks = 0;
    int memSize = 0; // memory allocated to this process (bytes)
    std::vector<std::string> declaredVars; // <-- Add this line

public:
    //added
    uint16_t getVariable(const std::string& name) const;
    void setVariable(const std::string& name, uint16_t value);
    //end added
    void setCpuId(int id) { cpuId = id; }
    int getCpuId() const { return cpuId; }
    void addCommand(PrintCommand* cmd) { commands.push_back(cmd); }
    void createPrintCommands(int totalIns);
    void executeCurrentCommand(int cpuId, std::string processName, std::string time);
    void addInstruction(Instruction* instr) { instructions.push_back(instr); }

    Process();
    Process(int pid, const std::string& name, int currentLine, int totalLines, const std::string& timestamp, const std::string& status, int memSize);
    Process(int pid, std::string processName, int memSize);
    ~Process() = default;


    int getPid() const { return pid; }
    const std::string& getName() const { return name; }
    int getCurrentLine() const { return currentLine; }
    int getTotalLines() const { return totalLines; }
    const std::string& getTimestamp() const { return timestamp; }
    const std::string& getStatus() const { return status; }
    int getMemSize() const { return memSize; }
    void setMemSize(int size) { memSize = size; }
    int getUsedMemory();
    int getEndAddress() const;
    int getInstructionSize(const std::string& instr);
    std::string generateRandomInstruction(int nestingLevel, int& instrBytes, int maxNesting = 3);

    void setStatus(const std::string& newStatus) { status = newStatus; };
    void moveCurrentLine();
    std::time_t getStartTime() const;
    void setEndTime(std::string t) { endTime = t; };
    std::string getEndTime() const { return endTime; };
    std::string getEndTimeString();
    std::vector<std::string> getAllLogs() const {
        std::vector<std::string> logs;
        for (const auto* cmd : commands) {
            logs.push_back(cmd->getLog());
        }
        return logs;
    }
    void setPid(int id){ pid = id; };

    // RR/OS-style helpers
    bool isFinished() const { return currentLine >= totalLines; }
    std::vector<std::string> getAllInstructions() const {
        std::vector<std::string> instrs;
        for (const auto* cmd : commands) {
            instrs.push_back(cmd->getInstruction());
        }
        return instrs;
    }
    std::vector<std::string>& getDeclaredVars() { return declaredVars; } // Optional getter
};

