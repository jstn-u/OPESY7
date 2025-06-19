#include "FCFSScheduler.h"
#include <algorithm>
#include <chrono>
#include <set>

FCFSScheduler::FCFSScheduler(int numCores) : numCores(numCores), running(false) {}

FCFSScheduler::~FCFSScheduler() {
    stop();
}

void FCFSScheduler::addProcess(Process* proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyProcesses.push_back(proc);
    readyQueue.push(proc);
    cv.notify_all();
}

void FCFSScheduler::start() {
    running = true;
    availableCores.clear();
    for (int i = 0; i < numCores; ++i) {
        availableCores.insert(i);
    }
    for (int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(&FCFSScheduler::cpuWorker, this, i);
    }
}

void FCFSScheduler::stop() {
    running = false;
    cv.notify_all();
    for (auto& t : cpuThreads) {
        if (t.joinable()) t.join();
    }
    cpuThreads.clear();
}

bool FCFSScheduler::isRunning() const {
    return running;
}

std::vector<Process*> FCFSScheduler::getRunningProcesses() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return runningProcesses;
}

std::vector<Process*> FCFSScheduler::getFinishedProcesses() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return finishedProcesses;
}

std::vector<Process*> FCFSScheduler::getReadyProcesses() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return readyProcesses;
}

void FCFSScheduler::cpuWorker(int coreId) {
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
                // Always pick the lowest-numbered available core
                assignedCore = *availableCores.begin();
                availableCores.erase(availableCores.begin());
                runningProcesses.push_back(proc);
            }
        }
        if (proc && assignedCore != -1) {
            proc->setCpuId(assignedCore);
            while (proc->getCurrentLine() < proc->getTotalLines()) {
                proc->executeCurrentCommand(assignedCore, proc->getName(), "");
                proc->moveCurrentLine();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            std::time_t now = std::time(nullptr);
            std::tm localTime;
#ifdef _WIN32
            localtime_s(&localTime, &now);
#else
            localtime_r(&now, &localTime);
#endif
            std::ostringstream oss;
            oss << std::put_time(&localTime, "%m/%d/%Y, %I:%M:%S %p");

            proc->setEndTime(oss.str());
            proc->setStatus("Finished");
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                finishedProcesses.push_back(proc);
                runningProcesses.erase(std::remove(runningProcesses.begin(), runningProcesses.end(), proc), runningProcesses.end());
                // Mark this core as available again
                availableCores.insert(assignedCore);
                cv.notify_all();
            }
        }
    }
}