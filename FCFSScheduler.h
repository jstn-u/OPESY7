#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <set>
#include "Process.h"

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

private:
    void schedulerThreadFunc();
    void cpuWorker(int coreId);

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
};