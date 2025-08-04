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

class FCFSScheduler {
public:
    FCFSScheduler(int numCores);
    ~FCFSScheduler();

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
    void printVMStat();
    void printProcessSMI();
    int getActiveTicks() const { return activeTicks.load(); }
    int getIdleTicks() const { return idleTicks.load(); }
    float getCpuUtilization();
    int getBusyCores();
    int getAvailableCores();

private:
    void schedulerThreadFunc();
    void cpuWorker(int coreId);
    void processGeneratorFunc();
    std::string getCurrentTimestamp();

    int numCores;
    std::vector<std::thread> cpuThreads;
    std::thread schedulerThread;
    std::queue<Process*> readyQueue;
    std::vector<Process*> runningProcesses;
    std::vector<Process*> finishedProcesses;
    std::vector<Process*> readyProcesses;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> running;
    std::set<int> availableCores;
    MemoryManager* memoryManager;
    std::atomic<uint32_t> cpuCycles{0};
    std::atomic<int> activeTicks{0};
    std::atomic<int> idleTicks{0};
    std::atomic<bool> processGenActive{false};
    std::thread processGeneratorThread;
    int batchProcessFreq = 0;
};