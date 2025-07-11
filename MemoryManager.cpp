#include "MemoryManager.h"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <iostream>

MemoryManager::MemoryManager(int totalMem, int memPerProc)
    : totalMem(totalMem), memPerProc(memPerProc) {
    memory.push_back({0, totalMem, false, ""});
}

bool MemoryManager::allocate(const std::string& procName) {
    for (size_t i = 0; i < memory.size(); ++i) {
        MemoryBlock& block = memory[i];
        if (!block.allocated && block.size >= memPerProc) {
            if (block.size > memPerProc) {
                MemoryBlock newBlock = {block.start + memPerProc, block.size - memPerProc, false, ""};
                block.size = memPerProc;
                memory.insert(memory.begin() + i + 1, newBlock);
            }
            block.allocated = true;
            block.processName = procName;
            return true;
        }
    }
    return false;
}

void MemoryManager::free(const std::string& procName) {
    for (auto& block : memory) {
        if (block.allocated && block.processName == procName) {
            block.allocated = false;
            block.processName = "";
        }
    }
    mergeFree();
}

void MemoryManager::mergeFree() {
    for (auto it = memory.begin(); it != memory.end(); ++it) {
        if (!it->allocated && it + 1 != memory.end() && !(it + 1)->allocated) {
            it->size += (it + 1)->size;
            memory.erase(it + 1);
            --it;
        }
    }
}

int MemoryManager::externalFragmentation() {
    int frag = 0;
    for (const auto& block : memory) {
        if (!block.allocated) frag += block.size;
    }
    return frag;
}

void MemoryManager::printSnapshot(int quantum) {
    std::ofstream out("memory_stamp_" + std::to_string(quantum) + ".txt");
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    out << "Timestamp: (" << std::put_time(&tm, "%m/%d/%Y %I:%M:%S%p") << ")\n";

    int active = 0;
    for (const auto& block : memory) {
        if (block.allocated) active++;
    }
    out << "Number of processes in memory: " << active << "\n";
    out << "Total external fragmentation in KB: " << externalFragmentation() << "\n\n";
    out << "----end---- = " << totalMem << "\n\n";

    for (const auto& block : memory) {
        out << block.start + block.size << "\n";
        if (block.allocated) {
            out << block.processName << "\n";
        }
    }
    out << "\n----start---- = 0\n";
    out.close();
}
