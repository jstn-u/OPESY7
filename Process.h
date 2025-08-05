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
#include <unordered_set>
#include "PrintCommand.h"

struct var_map {
    uint16_t value;
    std::string varname;
    bool declared = false;
};

class Instruction;

class Process {
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
    int n = 0;

public:
    std::unordered_map<std::string, std::uint16_t> declaredVarss;
    // "var1" = 1
    // "var2" = 3

    std::unordered_map<std::string, std::uint16_t> memoryAddSpace = {
    {"0x100", 0},
    {"0x200", 0},
    {"0x300", 0},
    {"0x400", 0},
    {"0x500", 0},
    {"0x600", 0},
    {"0x700", 0},
    {"0x800", 0},
    {"0x900", 0},
    {"0x1000", 0}
    };

    std::vector<std::vector<std::string>> commandOfStrings;
    bool isVar(std::string s);
    bool isMemAdd(std::string s);
    bool isVal(std::string s);
    void createStringCommands();
    std::vector<std::string> printStatements; // logs
    void logPrint(std::string log);
    void printLog();
    std::vector<std::vector<std::string>> stringCommands;
    void executeCurrentCommand2();
    //std::uint32_t currentLine = 0;
    //std::string pName;

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

    // For screen -c
    Process(int pid, const std::string& name, int currentLine, const std::string& timestamp, const std::string& status, int memSize, std::vector<std::vector<std::string>> commandS);

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
    std::string parseInstructions(std::string& input);

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
    void setPid(int id) { pid = id; };

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