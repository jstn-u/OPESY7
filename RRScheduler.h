#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <set>
#include "Process.h"
#include "MemoryManager.h"

class RRScheduler {
public:
    RRScheduler(int numCores, int quantumCycles);
    ~RRScheduler();

    void addProcess(Process* proc);
    void start();
    void stop();
    bool isRunning() const;
    std::vector<Process*> getRunningProcesses();
    std::vector<Process*> getFinishedProcesses();
    std::vector<Process*> getReadyProcesses();
    void startProcessGenerator(int batchFreq);
    void stopProcessGenerator();
    uint32_t getCpuCycles();
    void schedulerLoop();
    float getCpuUtilization();
    int getBusyCores();
    int getAvailableCores();

private:
    MemoryManager* memoryManager;
    void cpuWorker(int coreId);
    void processGeneratorFunc();
    std::string getCurrentTimestamp();

    int numCores;
    int quantumCycles;
    std::vector<std::thread> cpuThreads;
    std::queue<Process*> readyQueue;
    std::vector<Process*> runningProcesses;
    std::vector<Process*> finishedProcesses;
    std::vector<Process*> readyProcesses;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<uint32_t> cpuCycles{0};
    std::atomic<bool> running;
    std::set<int> availableCores;
    std::atomic<bool> processGenActive{false};
    std::thread processGeneratorThread;
    int batchProcessFreq = 0;
    bool tickRunning = false;
    std::thread tickThread;
    std::thread schedulerThread;
};
