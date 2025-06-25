#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <set>
#include "Process.h"

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

private:
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
    std::atomic<bool> running;
    std::set<int> availableCores;
    std::atomic<bool> processGenActive{false};
    std::atomic<int> cpuTick{0};
    std::thread processGeneratorThread;
    int batchProcessFreq = 0;
};
