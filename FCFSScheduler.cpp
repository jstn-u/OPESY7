#include "FCFSScheduler.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <set>

extern int delay_per_exec;
extern int num_cpu;
extern int curr_id;
extern int min_ins;
extern int max_ins;

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
            uint32_t currentCycleCount = globalCpuCycles.load();
            uint32_t targetCycleCount = currentCycleCount + delay_per_exec;
            while (proc->getCurrentLine() < proc->getTotalLines()) {
                // Busy-wait for delay_per_exec cycles
                while (globalCpuCycles.load() < targetCycleCount) {
                    // Optionally sleep to avoid tight loop
                    // std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
                proc->executeCurrentCommand(assignedCore, proc->getName(), "");
                proc->moveCurrentLine();
                targetCycleCount += delay_per_exec;
                globalCpuCycles++; // Increment global cycle counter after each instruction
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

float FCFSScheduler::getCpuUtilization() {
    int totalCores = numCores;
    int busyCores = 0;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        // If scheduler hasn't started, all cores are available
        if (!running) {
            busyCores = 0;
        } else {
            busyCores = totalCores - static_cast<int>(availableCores.size());
        }
    }
    if (totalCores == 0) return 0.0f;
    float cpuUtilization = (static_cast<float>(busyCores) * 100.0f) / totalCores;
    return cpuUtilization;
}

int FCFSScheduler::getBusyCores() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!running) return 0;
    return numCores - static_cast<int>(availableCores.size());
}

void FCFSScheduler::startProcessGenerator(int batchFreq) {
    batchProcessFreq = batchFreq;
    processGenActive = true;
    cpuTick = 0;
    processGeneratorThread = std::thread(&FCFSScheduler::processGeneratorFunc, this);
}

void FCFSScheduler::stopProcessGenerator() {
    processGenActive = false;
    if (processGeneratorThread.joinable()) processGeneratorThread.join();
}

std::string FCFSScheduler::getCurrentTimestamp() {
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

void FCFSScheduler::processGeneratorFunc() {
    while (processGenActive) {
        if (batchProcessFreq > 0 && cpuTick % batchProcessFreq == 0) {
            int totalInstructions = min_ins + (std::rand() % (max_ins - min_ins + 1));
            std::string processName = "auto_proc_" + std::to_string(curr_id);
            std::string timestamp = getCurrentTimestamp();
            Process* newProcess = new Process(curr_id, processName, 0, totalInstructions, timestamp, "Ready");
            curr_id++;
            newProcess->createPrintCommands(totalInstructions);
            addProcess(newProcess);
        }
    }
}

int FCFSScheduler::getAvailableCores() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!running) return numCores;
    return static_cast<int>(availableCores.size());
}