#include "RRScheduler.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <set>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <atomic>

extern int delay_per_exec;
extern int num_cpu;
extern int curr_id;
extern int min_ins;
extern int max_ins;

RRScheduler::RRScheduler(int numCores, int quantumCycles)
    : numCores(numCores), quantumCycles(quantumCycles), running(false), processGenActive(false), cpuCycles(0) {}

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

    // Start CPU core threads
    availableCores.clear();
    for (int i = 0; i < numCores; ++i) {
        availableCores.insert(i);
    }
    for (int i = 0; i < numCores; ++i) {
        cpuThreads.emplace_back(&RRScheduler::cpuWorker, this, i);
    }

    schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);
}

void RRScheduler::stop() {
    running = false;
    processGenActive = false;
    cv.notify_all();

    for (auto& t : cpuThreads) {
        if (t.joinable()) t.join();
    }
    cpuThreads.clear();

    if (schedulerThread.joinable()) schedulerThread.join();
    if (processGeneratorThread.joinable()) processGeneratorThread.join();
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

uint32_t RRScheduler::getCpuCycles() {
    return cpuCycles.load();
}

void RRScheduler::schedulerLoop() {
    while (running) {
        cpuCycles++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

float RRScheduler::getCpuUtilization() {
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

void RRScheduler::cpuWorker(int coreId) {
    //uint32_t currentCycleCount = getCpuCycles();
    //uint32_t targetCycleCount = currentCycleCount + delay_per_exec;
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
                // Simulate delay before instruction execution
                if (delay_per_exec > 0) {
                    auto start = std::chrono::steady_clock::now();
                    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(delay_per_exec)) {
                        // Busy-wait: simulate CPU being held
                    }
                }

                /* currentCycleCount = getCpuCycles();
                if(delay_per_exec > 0){
                    while(currentCycleCount < targetCycleCount){} // empty loop to wait for the next cycle
                } */

                proc->executeCurrentCommand(assignedCore, proc->getName(), "");
                proc->moveCurrentLine();
                quantum++;
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
    uint32_t lastCycle = 0;

    while (processGenActive) {
        uint32_t cycle = getCpuCycles();

        if (batchProcessFreq > 0 && cycle % batchProcessFreq == 0 && cycle != lastCycle) {
            lastCycle = cycle;

            int totalInstructions = min_ins + (std::rand() % (max_ins - min_ins + 1));
            std::string processName = "auto_proc_" + std::to_string(curr_id);
            std::string timestamp = getCurrentTimestamp();

            Process* newProcess = new Process(curr_id, processName, 0, totalInstructions, timestamp, "Ready");
            curr_id++;
            newProcess->createPrintCommands(totalInstructions);
            addProcess(newProcess);
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(1));  // avoid tight loop
    }
}

int RRScheduler::getBusyCores() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!running) return 0;
    return numCores - static_cast<int>(availableCores.size());
}

int RRScheduler::getAvailableCores() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!running) return numCores;
    return static_cast<int>(availableCores.size());
}
