#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <shared_mutex>

// --- Page Table Entry ---
struct PageTableEntry {
    int frameNumber = -1;
    bool valid = false;
    bool dirty = false;
};

// --- Frame structure ---
struct Frame {
    int frameNumber;
    bool occupied;
    std::string processName;
    int pageNumber;
    bool dirty;
};

// --- Memory Block (for external fragmentation & snapshotting) ---
struct MemoryBlock {
    int start;
    int size;
    bool allocated;
    std::string processName;
};

class Process;

class MemoryManager {
public:
    MemoryManager(int totalMem, int memPerProc, int memPerFrame);

    // Core process memory management
    void accessPage(const std::string& procName, int pageNumber);
    void handlePageFault(const std::string& procName, int pageNumber);
    void freeProcessMemory(const std::string& procName);

    // Logging & stats
    void logProcessMetadataToBackingStore(const Process* proc);
    void printProcessSMI();
    void printVMStat(uint32_t cpuCycles, int idleTicks, int activeTicks);
    void printFrames();
    void printSnapshot(int quantum);
    int externalFragmentation();
    const std::unordered_map<std::string, std::vector<PageTableEntry>>& getPageTables() const { return pageTables; }
    const std::vector<Frame>& getFrames() const { return frames; }
    void loadPageFromBackingStore(const std::string& procName, int pageNumber, int frameNumber);
    void evictPageToBackingStore(const std::string& procName, int pageNumber, int frameNumber);
    /*
    bool allocate(const std::string& procName);
    void free(const std::string& procName);
    void mergeFree();
    */

private:
    mutable std::shared_mutex memoryMutex;

    // Frame management
    int findFreeFrame();
    int selectVictimFrame();

    // Internal state
    int totalMem;
    int memPerProc;
    int memPerFrame;
    int numFrames;
    int pagesPagedIn;
    int pagesPagedOut;

    std::vector<Frame> frames;
    std::unordered_map<std::string, std::vector<PageTableEntry>> pageTables;

    std::vector<MemoryBlock> memory;

    const std::string backingStoreFile = "csopesy-backing_store.txt";
};
