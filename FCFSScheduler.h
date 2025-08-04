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
    std::atomic<bool> processGenActive{false};
    std::atomic<uint32_t> cpuCycles{0};
    std::atomic<uint32_t> globalCpuCycles{0};
    std::atomic<int> cpuTick{0};
    std::thread processGeneratorThread;
    int batchProcessFreq = 0;
    int curr_id;
    std::atomic<int> activeTicks{0};
    std::atomic<int> idleTicks{0};
};