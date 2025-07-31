#include "RRScheduler.h"
#include "MemoryManager.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <set>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <thread>
#include <atomic>
#include <iostream>

extern int delay_per_exec;
extern int num_cpu;
extern int curr_id;
extern int min_ins;
extern int max_ins;
extern int min_mem_per_proc;
extern int max_mem_per_proc;
extern int max_overall_mem;
extern int mem_per_frame;

/*RRScheduler::RRScheduler(int numCores, int quantumCycles)
    : numCores(numCores), quantumCycles(quantumCycles), running(false), processGenActive(false), cpuCycles(0) {}

RRScheduler::~RRScheduler() {
    stop();
}*/

RRScheduler::RRScheduler(int numCores, int quantumCycles)
    : numCores(numCores), quantumCycles(quantumCycles), running(false), processGenActive(false), cpuCycles(0) {
    // Use max_mem_per_proc for MemoryManager, but each process will have its own memSize
    memoryManager = new MemoryManager(max_overall_mem, max_mem_per_proc, mem_per_frame);
}

RRScheduler::~RRScheduler() {
    stop();
    delete memoryManager;
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

    //schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);
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
    static std::set<std::string> allocatedSet;
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
            uint32_t targetCycle = cpuCycles.load() + delay_per_exec;

            while (quantum < quantumCycles && proc->getCurrentLine() < proc->getTotalLines()) {
                // Demand paging: determine which page to access based on current instruction line
                // Assume 1 instruction per 2 bytes, and mem_per_frame is in bytes
                int bytesPerInstruction = 2; // Each instruction is 2 bytes (simulate)
                int address = proc->getCurrentLine() * bytesPerInstruction;
                int pageNumber = address / mem_per_frame;
                memoryManager->accessPage(proc->getName(), pageNumber);

                // Wait until cpuCycles reaches targetCycle (simulate delay_per_exec)
                if (delay_per_exec > 0) {
                    while (cpuCycles.load() < targetCycle) {
                        cpuCycles++;
                    }
                }

                proc->executeCurrentCommand(assignedCore, proc->getName(), "");
                proc->moveCurrentLine();

                cpuCycles++;
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
                    //memoryManager->logProcessMetadataToBackingStore(proc);
                    memoryManager->freeProcessMemory(proc->getName());
                    allocatedSet.erase(proc->getName());
                } else {
                    readyQueue.push(proc);
                }

                availableCores.insert(assignedCore);
                cv.notify_all();
            }
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(25));
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
    uint32_t lastCycle = -1;
    while (processGenActive) {
        uint32_t cycle = getCpuCycles();
        if (batchProcessFreq > 0 && cycle % batchProcessFreq == 0 && lastCycle != cycle) {
            int mem_for_proc = min_mem_per_proc + (std::rand() % (max_mem_per_proc - min_mem_per_proc + 1));
            int numPages = ceil(mem_for_proc / mem_per_frame);
            int totalFrames = max_overall_mem / mem_per_frame;
            int usedFrames = 0;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                // Count frames used by all processes in page tables
                for (const auto& pt : memoryManager->getPageTables()) {
                    for (const auto& entry : pt.second) {
                        if (entry.valid) ++usedFrames;
                    }
                }
            }
            // Checks if there are enough frames available
            if (usedFrames + numPages <= totalFrames) {
                int totalInstructions = min_ins + (std::rand() % (max_ins - min_ins + 1));
                std::string processName = "auto_proc_" + std::to_string(curr_id);
                std::string timestamp = getCurrentTimestamp();

                Process* newProcess = new Process(curr_id, processName, 0, totalInstructions, timestamp, "Ready", mem_for_proc);
                curr_id++;
                newProcess->createPrintCommands(totalInstructions);
                addProcess(newProcess);

                // Force allocation of all pages for this process (for debugging and correctness)
                for (int page = 0; page < numPages; ++page) {
                    memoryManager->accessPage(newProcess->getName(), page);
                }

                lastCycle = cycle;
                //std::cout << "[DEBUG] Created process: " << processName << " at cycle " << cycle << std::endl;
            }else{
                // Not enough frames: create process and immediately evict all its pages to backing store
                int totalInstructions = min_ins + (std::rand() % (max_ins - min_ins + 1));
                std::string processName = "auto_proc_" + std::to_string(curr_id);
                std::string timestamp = getCurrentTimestamp();

                Process* newProcess = new Process(curr_id, processName, 0, totalInstructions, timestamp, "Swapped", mem_for_proc);
                curr_id++;
                newProcess->createPrintCommands(totalInstructions);
                addProcess(newProcess);

                // Evict all pages to backing store
                for (int page = 0; page < numPages; ++page) {
                    memoryManager->evictPageToBackingStore(newProcess->getName(), page, -1);
                }
            }

            // Visualization: print frame table after each process creation attempt
            /* {
                const auto& frames = memoryManager->getFrames();
                std::cout << "\n[Frame Table Visualization] (cycle " << cycle << ")\n";
                std::cout << "Frame | Occupied | Process | Page\n";
                std::cout << "-----------------------------------\n";
                for (const auto& frame : frames) {
                    std::cout << frame.frameNumber << "\t" << (frame.occupied ? "Yes" : "No") << "\t";
                    if (frame.occupied) {
                        std::cout << frame.processName << "\t" << frame.pageNumber;
                    } else {
                        std::cout << "-\t-";
                    }
                    std::cout << "\n";
                }
                std::cout << "-----------------------------------\n";
            } */
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(1)); // avoid tight loop
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