
#include "MemoryManager.h"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <algorithm>

MemoryManager::MemoryManager(int totalMem, int memPerProc, int memPerFrame)
    : totalMem(totalMem), memPerProc(memPerProc), memPerFrame(memPerFrame) {
    numFrames = totalMem / memPerFrame;
    for (int i = 0; i < numFrames; ++i) {
        frames.push_back({i, false, "", -1, false});
    }
}

void MemoryManager::accessPage(const std::string& procName, int pageNumber) {
    // If page is not in memory, handle page fault
    if (pageTables[procName].size() <= pageNumber || !pageTables[procName][pageNumber].valid) {
        handlePageFault(procName, pageNumber);
    }
    // else, page is in memory, access proceeds
}

void MemoryManager::handlePageFault(const std::string& procName, int pageNumber) {
    int freeFrame = findFreeFrame();
    if (freeFrame == -1) {
        freeFrame = selectVictimFrame();
        // Evict victim
        Frame& victim = frames[freeFrame];
        if (victim.occupied) {
            evictPageToBackingStore(victim.processName, victim.pageNumber, freeFrame);
            pageTables[victim.processName][victim.pageNumber].valid = false;
        }
    }
    // Load the required page into the frame
    loadPageFromBackingStore(procName, pageNumber, freeFrame);
    frames[freeFrame].occupied = true;
    frames[freeFrame].processName = procName;
    frames[freeFrame].pageNumber = pageNumber;
    frames[freeFrame].dirty = false;
    // Update page table
    if (pageTables[procName].size() <= pageNumber) pageTables[procName].resize(pageNumber + 1);
    pageTables[procName][pageNumber].frameNumber = freeFrame;
    pageTables[procName][pageNumber].valid = true;
    pageTables[procName][pageNumber].dirty = false;
}

void MemoryManager::loadPageFromBackingStore(const std::string& procName, int pageNumber, int frameNumber) {
    // Simulate loading a page from the backing store file
    //std::cout << "[DemandPaging] Loading page " << pageNumber << " of process " << procName << " into frame " << frameNumber << std::endl;
    // TODO: Implement actual file I/O
}

void MemoryManager::evictPageToBackingStore(const std::string& procName, int pageNumber, int frameNumber) {
    // Simulate writing a page to the backing store file
    //std::cout << "[DemandPaging] Evicting page " << pageNumber << " of process " << procName << " from frame " << frameNumber << std::endl;
    // TODO: Implement actual file I/O
}

int MemoryManager::findFreeFrame() {
    for (auto& frame : frames) {
        if (!frame.occupied) return frame.frameNumber;
    }
    return -1;
}

int MemoryManager::selectVictimFrame() {
    // Simple FIFO: pick the first occupied frame
    for (auto& frame : frames) {
        if (frame.occupied) return frame.frameNumber;
    }
    return 0; // fallback
}

void MemoryManager::printFrames() {
    std::cout << "[Frames]" << std::endl;
    for (const auto& frame : frames) {
        std::cout << "Frame " << frame.frameNumber << ": ";
        if (frame.occupied) {
            std::cout << frame.processName << " page " << frame.pageNumber;
        } else {
            std::cout << "free";
        }
        std::cout << std::endl;
    }
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