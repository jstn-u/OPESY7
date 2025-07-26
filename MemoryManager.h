#pragma once
#include <string>
#include <vector>
#include <map>

struct MemoryBlock {
    int start;
    int size;
    bool allocated;
    std::string processName;
};

struct Frame {
    int frameNumber;
    bool occupied;
    std::string processName;
    int pageNumber; // which virtual page is loaded here
    bool dirty;
};

struct PageTableEntry {
    int frameNumber; // -1 if not in memory
    bool valid;
    bool dirty;
};

class MemoryManager {
public:
    // Getter for frames (for visualization/debugging)
    const std::vector<Frame>& getFrames() const { return frames; }
    // Getter for pageTables (for scheduler memory checks)
    const std::map<std::string, std::vector<PageTableEntry>>& getPageTables() const { return pageTables; }
    MemoryManager(int totalMem, int memPerProc, int memPerFrame);

    // Demand paging interface
    void accessPage(const std::string& procName, int pageNumber);
    void handlePageFault(const std::string& procName, int pageNumber);
    void loadPageFromBackingStore(const std::string& procName, int pageNumber, int frameNumber);
    void evictPageToBackingStore(const std::string& procName, int pageNumber, int frameNumber);
    int findFreeFrame();
    int selectVictimFrame(); // for page replacement
    void printFrames();

    // Old interface (for compatibility, can be removed later)
    bool allocate(const std::string& procName);
    void free(const std::string& procName);
    void mergeFree();
    int externalFragmentation();
    void printSnapshot(int quantum);

private:
    std::vector<MemoryBlock> memory;
    std::vector<Frame> frames;
    int totalMem;
    int memPerProc;
    int memPerFrame;
    int numFrames;
    std::map<std::string, std::vector<PageTableEntry>> pageTables; // processName -> page table
    std::string backingStoreFile = "csopesy-backing-store.txt";
};
