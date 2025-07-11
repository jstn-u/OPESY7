#include "MemoryManager.h"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <algorithm>

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

    // Print each allocated block in descending order of start
    std::vector<MemoryBlock> allocatedBlocks;
    for (const auto& block : memory) {
        if (block.allocated) allocatedBlocks.push_back(block);
    }
    // Sort by start descending
    std::sort(allocatedBlocks.begin(), allocatedBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.start > b.start;
    });
    for (const auto& block : allocatedBlocks) {
        int upper = block.start + block.size;
        int lower = block.start;
        std::string px = "P";
        size_t pos = block.processName.find_last_of('_');
        if (pos != std::string::npos && pos + 1 < block.processName.size()) {
            px += block.processName.substr(pos + 1);
        } else {
            px += block.processName; // fallback
        }
        out << upper << "\n";
        out << px << "\n";
        out << lower << "\n\n";
    }
    out << "\n----start---- = 0\n";
    out.close();
}
