#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <chrono>
#include <climits>

class Process; 


class Instruction {
public:
    virtual ~Instruction() = default;
    virtual void execute(Process* process) = 0;
};


class PrintInstruction : public Instruction {
    std::string message;
public:
    PrintInstruction(const std::string& msg) : message(msg) {}
    void execute(Process* process) override;
};


class DeclareInstruction : public Instruction {
    std::string varName;
    uint16_t value;
public:
    DeclareInstruction(const std::string& name, uint16_t val) : varName(name), value(val) {}
    void execute(Process* process) override;
};


class AddInstruction : public Instruction {
    std::string target;
    std::string src1;
    std::string src2;
public:
    AddInstruction(const std::string& tgt, const std::string& s1, const std::string& s2)
        : target(tgt), src1(s1), src2(s2) {}
    void execute(Process* process) override;
};


class SubtractInstruction : public Instruction {
    std::string target;
    std::string src1;
    std::string src2;
public:
    SubtractInstruction(const std::string& tgt, const std::string& s1, const std::string& s2)
        : target(tgt), src1(s1), src2(s2) {}
    void execute(Process* process) override;
};


class SleepInstruction : public Instruction {
    uint8_t ticks;
public:
    SleepInstruction(uint8_t t) : ticks(t) {}
    void execute(Process* process) override;
};


class ForInstruction : public Instruction {
    std::vector<Instruction*> innerInstructions;
    int repeats;
public:
    ForInstruction(const std::vector<Instruction*>& ins, int rpt) : innerInstructions(ins), repeats(rpt) {}
    void execute(Process* process) override;
};
