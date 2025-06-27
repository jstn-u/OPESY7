#include "Process.h"
#include "Instructions.h"

void PrintInstruction::execute(Process* process) {
    std::cout << "Hello world from " << process->getName() << "!" << std::endl;
}

void DeclareInstruction::execute(Process* process) {
    process->setVariable(varName, value);
}

void AddInstruction::execute(Process* process) {
    uint16_t val1 = process->getVariable(src1);
    uint16_t val2 = process->getVariable(src2);
    uint32_t result = static_cast<uint32_t>(val1) + static_cast<uint32_t>(val2);
    process->setVariable(target, std::min<uint32_t>(result, UINT16_MAX));
}

void SubtractInstruction::execute(Process* process) {
    uint16_t val1 = process->getVariable(src1);
    uint16_t val2 = process->getVariable(src2);
    int result = static_cast<int>(val1) - static_cast<int>(val2);
    process->setVariable(target, result < 0 ? 0 : result);
}

void SleepInstruction::execute(Process* process) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ticks * 10));
    // Assuming 1 CPU tick = 10 ms, adjust as needed
}

void ForInstruction::execute(Process* process) {
    for (int i = 0; i < repeats; ++i) {
        for (Instruction* instr : innerInstructions) {
            instr->execute(process);
        }
    }
}
