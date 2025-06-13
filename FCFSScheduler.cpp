#include "FCFSScheduler.h"
#include <algorithm>
#include <chrono>

FCFSScheduler::FCFSScheduler(int numCores) : numCores(numCores), running(false) {}

FCFSScheduler::~FCFSScheduler() {
    stop();
}

void FCFSScheduler::addProcess(Process* proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(proc);
    cv.notify_all();
}

void FCFSScheduler::start() {
    running = true;
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

void FCFSScheduler::cpuWorker(int coreId) {
    while (running) {
        Process* proc = nullptr;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] { return !readyQueue.empty() || !running; });
            if (!running) break;
            if (!readyQueue.empty()) {
                proc = readyQueue.front();
                readyQueue.pop();
                runningProcesses.push_back(proc);
            }
        }
        if (proc) {
            proc->setCpuId(coreId);
            while (proc->getCurrentLine() < proc->getTotalLines()) {
                proc->executeCurrentCommand(coreId, proc->getName(), "");
                proc->moveCurrentLine();
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
            proc->setStatus("Finished");
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                finishedProcesses.push_back(proc);
                runningProcesses.erase(std::remove(runningProcesses.begin(), runningProcesses.end(), proc), runningProcesses.end());
            }
        }
    }
}