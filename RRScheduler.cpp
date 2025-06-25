#include "RRScheduler.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <set>
#include <sstream>
#include <iomanip>
#include <ctime>

extern int delay_per_exec;
extern int curr_id;

RRScheduler::RRScheduler(int numCores, int quantumCycles)
    : numCores(numCores), quantumCycles(quantumCycles), running(false) {}

RRScheduler::~RRScheduler() {
    stop();
}

void RRScheduler::addProcess(Process* proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyProcesses.push_back(proc);
    readyQueue.push(proc);
    cv.notify_all();
}

void RRScheduler::start() {
    running = true;
    availableCores.clear();
    for (int i = 0; i < numCores; ++i) {
        availableCores.insert(i);
    }
    for (int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(&RRScheduler::cpuWorker, this, i);
    }
}

void RRScheduler::stop() {
    running = false;
    cv.notify_all();
    for (auto& t : cpuThreads) {
        if (t.joinable()) t.join();
    }
    cpuThreads.clear();
}

bool RRScheduler::isRunning() const {
    return running;
}

std::vector<Process*> RRScheduler::getRunningProcesses() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return runningProcesses;
}

std::vector<Process*> RRScheduler::getFinishedProcesses() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return finishedProcesses;
}

std::vector<Process*> RRScheduler::getReadyProcesses() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return readyProcesses;
}

void RRScheduler::cpuWorker(int coreId) {
    while (running) {
        Process* proc = nullptr;
        int assignedCore = -1;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] { return (!readyQueue.empty() && !availableCores.empty()) || !running; });
            if (!running) break;
            if (!readyQueue.empty() && !availableCores.empty()) {
                proc = readyQueue.front();
                readyQueue.pop();
                assignedCore = *availableCores.begin();
                availableCores.erase(availableCores.begin());
                runningProcesses.push_back(proc);
            }
        }
        if (proc && assignedCore != -1) {
            proc->setCpuId(assignedCore);
            int quantum = 0;
            while (quantum < quantumCycles && proc->getCurrentLine() < proc->getTotalLines()) {
                proc->executeCurrentCommand(assignedCore, proc->getName(), "");
                proc->moveCurrentLine();
                ++quantum;
                if (delay_per_exec > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_per_exec));
                }
            }
            bool finished = proc->getCurrentLine() >= proc->getTotalLines();
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                runningProcesses.erase(std::remove(runningProcesses.begin(), runningProcesses.end(), proc), runningProcesses.end());
                if (finished) {
                    proc->setEndTime(getCurrentTimestamp());
                    proc->setStatus("Finished");
                    finishedProcesses.push_back(proc);
                } else {
                    readyQueue.push(proc);
                }
                availableCores.insert(assignedCore);
                cv.notify_all();
            }
        }
    }
}

void RRScheduler::startProcessGenerator(int batchFreq) {
    batchProcessFreq = batchFreq;
    processGenActive = true;
    cpuTick = 0;
    processGeneratorThread = std::thread(&RRScheduler::processGeneratorFunc, this);
}

void RRScheduler::stopProcessGenerator() {
    processGenActive = false;
    if (processGeneratorThread.joinable()) processGeneratorThread.join();
}

std::string RRScheduler::getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}

void RRScheduler::processGeneratorFunc() {
    int procNum = 0;
    while (processGenActive) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        int tick = ++cpuTick;
        if (batchProcessFreq > 0 && tick % batchProcessFreq == 0) {
            std::string processName = "auto_proc_" + std::to_string(procNum);
            std::string timestamp = getCurrentTimestamp();
            Process* newProcess = new Process(curr_id, processName, 0, 100, timestamp, "Ready");
            curr_id++;
            procNum++;
            newProcess->create100PrintCommands();
            addProcess(newProcess);
        }
    }
}
