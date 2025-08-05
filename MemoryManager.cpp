#include "MemoryManager.h"
#include "Process.h"
#include <fstream>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <sstream>

MemoryManager::MemoryManager(int totalMem, int memPerProc, int memPerFrame)
    : totalMem(totalMem), memPerProc(memPerProc), memPerFrame(memPerFrame), pagesPagedIn(0), pagesPagedOut(0) {
    numFrames = totalMem / memPerFrame;
    for (int i = 0; i < numFrames; ++i) {
        frames.push_back({i, false, "", -1, false});
    }
    // Clear the backing store file if it exists (start fresh)
    std::ofstream clearFile(backingStoreFile, std::ios::trunc);
    if (clearFile.is_open()) {
        clearFile.close();
    }
}

// Free all frames, page table entries, and backing store entries for a process
void MemoryManager::freeProcessMemory(const std::string& procName) {
    // 1. Free all frames belonging to this process
    std::unique_lock<std::shared_mutex> lock(memoryMutex);
    for (auto& frame : frames) {
        if (frame.occupied && frame.processName == procName) {
            frame.occupied = false;
            frame.processName = "";
            frame.pageNumber = -1;
            frame.dirty = false;
        }
    }
    // 2. Remove page table
    pageTables.erase(procName);
    // 3. Remove all lines for this process from the backing store file
    std::ifstream inFile(backingStoreFile);
    std::vector<std::string> lines;
    std::string line;
    std::string tagPrefix = procName + ":page";
    if (inFile.is_open()) {
        while (std::getline(inFile, line)) {
            // Only remove lines that start with procName:page (i.e., this process)
            if (line.rfind(tagPrefix, 0) != 0) {
                lines.push_back(line);
            }
        }
        inFile.close();
    }
    std::ofstream outFile(backingStoreFile, std::ios::trunc);
    if (outFile.is_open()) {
        for (const auto& l : lines) {
            outFile << l << "\n";
        }
        outFile.close();
    }
}

std::string MemoryManager::accessPage(const std::string& procName, int pageNumber) {
    // If page is not in memory, handle page fault
    std::shared_lock<std::shared_mutex> lock(memoryMutex);
    std::string victimProc = "";

    if (pageTables[procName].size() <= pageNumber || !pageTables[procName][pageNumber].valid) {
        lock.unlock();
        //std::cout << "[Page Fault] Process: " << procName << ", Page: " << pageNumber << std::endl;
        victimProc = handlePageFault(procName, pageNumber);
    }
    return victimProc;
}

std::string MemoryManager::handlePageFault(const std::string& procName, int pageNumber) {
    std::unique_lock<std::shared_mutex> lock(memoryMutex);
    int freeFrame = findFreeFrame();
    std::string victimProc = "";

    if (freeFrame == -1) {
        freeFrame = selectVictimFrame();
        // Evict victim
        Frame& victim = frames[freeFrame];
        if (victim.occupied) {
            evictPageToBackingStore(victim.processName, victim.pageNumber, freeFrame);
            pageTables[victim.processName][victim.pageNumber].valid = false;
            victimProc = victim.processName;
        }
    }
    // Load the required page into the frame
    loadPageFromBackingStore(procName, pageNumber, freeFrame);
    frames[freeFrame].occupied = true;
    frames[freeFrame].processName = procName;
    frames[freeFrame].pageNumber = pageNumber;
    frames[freeFrame].dirty = false;
    frames[freeFrame].loadTime = currentTick++;

    // Update page table
    if (pageTables[procName].size() <= pageNumber) pageTables[procName].resize(pageNumber + 1);
    pageTables[procName][pageNumber].frameNumber = freeFrame;
    pageTables[procName][pageNumber].valid = true;
    pageTables[procName][pageNumber].dirty = false;

    return victimProc;
}

void MemoryManager::loadPageFromBackingStore(const std::string& procName, int pageNumber, int frameNumber) {
    // Simulate loading a page from the backing store file
    std::stringstream outputBuffer;
    std::ifstream backingStore(backingStoreFile);
    std::string line;
    std::string pageData;
    std::string pageTag = procName + ":page" + std::to_string(pageNumber);

    bool found = false;
    if (backingStore.is_open()) {
        while (std::getline(backingStore, line)) {
            if (line.find(pageTag) == 0) {
                pageData = line.substr(pageTag.length());
                found = true;
                break;
            }
        }
        backingStore.close();
    }
    if (!found) {
        // If not found, just write the page tag (no data, no colon at end)
        static std::mutex backingStoreMutex;
        std::lock_guard<std::mutex> lock(backingStoreMutex);
        std::ofstream outFile(backingStoreFile, std::ios::app);
        if (outFile.is_open()) {
            outFile << pageTag << "\n";
            outFile.close();
        }
    }
    // For simulation, you could store pageData in a frameData map if needed
    pagesPagedIn++;
    //std::cout << "[BackingStore] Loaded page " << pageNumber << " of process " << procName << " into frame " << frameNumber << std::endl;
}

void MemoryManager::printVMStat(uint32_t cpuCycles, int idleTicks, int activeTicks) {
    int used = 0;
    for (const auto& frame : frames) {
        if (frame.occupied) used++;
    }
    std::cout << "\n=== vmstat ===\n";
    std::cout << "Total memory: " << totalMem << " bytes\n";
    std::cout << "Used memory: " << used * memPerFrame << " bytes\n";
    std::cout << "Free memory: " << (totalMem - used * memPerFrame) << " bytes\n";
    std::cout << "Idle CPU ticks: " << idleTicks << "\n";
    std::cout << "Active CPU ticks: " << activeTicks << "\n";
    std::cout << "Total CPU ticks: " << cpuCycles << "\n";
    std::cout << "Pages paged in: " << pagesPagedIn << "\n";
    std::cout << "Pages paged out: " << pagesPagedOut << "\n\n";
}

void MemoryManager::evictPageToBackingStore(const std::string& procName, int pageNumber, int frameNumber) {
    std::ifstream inFile(backingStoreFile);
    std::vector<std::string> lines; 
    std::string line;
    std::string pageTag = procName + ":page" + std::to_string(pageNumber) + ":";

    // Step 1: Read existing lines, excluding the old version of this page (if any)
    if (inFile.is_open()) {
        while (std::getline(inFile, line)) {
            if (line.find(pageTag) == 0) {
                continue;  // Skip existing version
            }
            lines.push_back(line);  // Keep other pages
        }
        inFile.close();
    }

    // Step 2: Retrieve actual frame data if desired (currently uses placeholder 'X')
    std::string pageData(memPerFrame, 'X'); // TODO: replace with actual data from frame if implemented

    // Step 3: Add new version of this page
    lines.push_back(pageTag + pageData);

    // Step 4: Thread-safe write to backing store
    {
        static std::mutex backingStoreMutex;
        std::lock_guard<std::mutex> lock(backingStoreMutex);

        std::ofstream outFile(backingStoreFile, std::ios::trunc);
        if (outFile.is_open()) {
            for (const auto& l : lines) {
                outFile << l << "\n";
            }
            outFile.close();
        }
    }

    // Step 5: Update stats
    pagesPagedOut++;
    //std::cout << "Pages Paged Out: " << pagesPagedOut << "\n";

    // Optional: Logging for debug
    /* std::cout << "[BackingStore] Evicted page " << pageNumber
              << " of process " << procName
              << " from frame " << frameNumber << std::endl; */
}

int MemoryManager::findFreeFrame() {
    for (auto& frame : frames) {
        if (!frame.occupied) return frame.frameNumber;
    }
    return -1;
}

int MemoryManager::selectVictimFrame() {
    int victim = -1;
    int oldestTime = INT_MAX;
    for (const auto& frame : frames) {
        if (frame.occupied && frame.loadTime < oldestTime) {
            oldestTime = frame.loadTime;
            victim = frame.frameNumber;
        }
    }
    return victim;
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

int MemoryManager::getUsedMemory() const {
    std::shared_lock lock(memoryMutex);
    int usedFrames = 0;
    for (const auto& frame : frames) {
        if (frame.occupied) {
            ++usedFrames;
        }
    }
    return usedFrames * memPerFrame;
}

int MemoryManager::getProcessMemoryUsage(const std::string& procName) const {
    std::shared_lock<std::shared_mutex> lock(memoryMutex);
    int count = 0;
    for (const auto& frame : frames) {
        if (frame.occupied && frame.processName == procName) {
            ++count;
        }
    }
    return count * memPerFrame;
}

/* bool MemoryManager::allocate(const std::string& procName) {
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
} */

/* void MemoryManager::printSnapshot(int quantum) {
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
} */