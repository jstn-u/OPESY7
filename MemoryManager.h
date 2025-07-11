#pragma once
#include <string>
#include <vector>

struct MemoryBlock {
    int start;
    int size;
    bool allocated;
    std::string processName;
};

class MemoryManager {
public:
    MemoryManager(int totalMem, int memPerProc);

    bool allocate(const std::string& procName);
    void free(const std::string& procName);
    void mergeFree();
    int externalFragmentation();
    void printSnapshot(int quantum);

private:
    std::vector<MemoryBlock> memory;
    int totalMem;
    int memPerProc;
};
