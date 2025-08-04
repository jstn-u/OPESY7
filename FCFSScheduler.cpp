#include "Process.h"
#include "FCFSScheduler.h"
#include "MemoryManager.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <set>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <ctime>
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

FCFSScheduler::FCFSScheduler(int numCores)
    : numCores(numCores), running(false), cpuCycles(0), activeTicks(0), idleTicks(0), processGenActive(false) {
    memoryManager = new MemoryManager(max_overall_mem, max_mem_per_proc, mem_per_frame);
}

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
    if (processGeneratorThread.joinable()) processGeneratorThread.join();
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
                assignedCore = *availableCores.begin();
                availableCores.erase(availableCores.begin());
                if (std::find(runningProcesses.begin(), runningProcesses.end(), proc) == runningProcesses.end()) {
                    runningProcesses.push_back(proc);
                }
            }
        }
        if (proc && assignedCore != -1 && proc->getStatus() == "Ready") {
            proc->setCpuId(assignedCore);
            int totalInstr = proc->getTotalLines();
            int cur_instr = proc->getCurrentLine();
            int numPages = static_cast<int>(std::ceil(static_cast<double>(proc->getMemSize()) / mem_per_frame));

            while (proc->getCurrentLine() < proc->getTotalLines()) {
                std::string victimProc = "";
                int address = proc->getCurrentLine();
                proc->setStatus("Running");
                int pageNumber = (address / mem_per_frame) - 1;
                if (pageNumber < 0) pageNumber = 0;

                memoryManager->accessPage(proc->getName(), pageNumber);

                if (delay_per_exec > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_per_exec));
                }

                proc->executeCurrentCommand(assignedCore, proc->getName(), "");

                //(to catch 100% or 0% cpu utilization)
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); 

                proc->moveCurrentLine();
                cpuCycles++;
                activeTicks++;
            }

            bool finished = proc->getCurrentLine() >= proc->getTotalLines();

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (finished) {
                    proc->setEndTime(getCurrentTimestamp());
                    proc->setStatus("Finished");
                    finishedProcesses.push_back(proc);
                    memoryManager->freeProcessMemory(proc->getName());
                }
                runningProcesses.erase(std::remove(runningProcesses.begin(), runningProcesses.end(), proc), runningProcesses.end());
                availableCores.insert(assignedCore);
                cv.notify_all();
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            cpuCycles++;
            idleTicks++;
        }
    }
}

float FCFSScheduler::getCpuUtilization() {
    int totalCores = numCores;
    int busyCores = 0;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
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
    processGeneratorThread = std::thread(&FCFSScheduler::processGeneratorFunc, this);
}

void FCFSScheduler::stopProcessGenerator() {
    processGenActive = false;
    if (processGeneratorThread.joinable()) processGeneratorThread.join();
}

uint32_t FCFSScheduler::getCpuCycles() {
    return cpuCycles.load();
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
    uint32_t lastCycle = -1;
    while (processGenActive && running) {
        uint32_t cycle = getCpuCycles();
        if (batchProcessFreq > 0 && cycle % batchProcessFreq == 0 && lastCycle != cycle) {
            int min_exp = static_cast<int>(std::log2(min_mem_per_proc));
            int max_exp = static_cast<int>(std::log2(max_mem_per_proc));
            int chosen_exp = min_exp + (std::rand() % (max_exp - min_exp + 1));
            int mem_for_proc = 1 << chosen_exp;

            int numPages = static_cast<int>(std::ceil(static_cast<double>(mem_for_proc) / mem_per_frame));
            int totalFrames = static_cast<int>(std::ceil(static_cast<double>(max_overall_mem) / mem_per_frame));
            int usedFrames = 0;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                for (const auto& pt : memoryManager->getPageTables()) {
                    for (const auto& entry : pt.second) {
                        if (entry.valid) ++usedFrames;
                    }
                }
            }

            int totalInstructions = min_ins + (std::rand() % (max_ins - min_ins + 1));
            std::string processName = "auto_proc_" + std::to_string(curr_id);
            std::string timestamp = getCurrentTimestamp();
            Process* newProcess = new Process(curr_id, processName, 0, totalInstructions, timestamp, "Ready", mem_for_proc);

            if (usedFrames + numPages <= totalFrames) {
                curr_id++;
                newProcess->createPrintCommands(totalInstructions);

                // Optionally, calculate totalInstrBytes and terminate if needed (see RR logic)
                /*
                int totalInstrBytes = 0;
                for (const std::string& cmd : newProcess->getAllInstructions()) {
                    totalInstrBytes += getInstructionSize(cmd);
                }
                if (totalInstrBytes > mem_for_proc) {
                    newProcess->setStatus("Terminated (Insufficient Memory)");
                    newProcess->setEndTime(getCurrentTimestamp());
                    finishedProcesses.push_back(newProcess);
                    continue;
                }
                */

                addProcess(newProcess);
                lastCycle = cycle;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        cpuCycles++;
        idleTicks++;
    }
}

int FCFSScheduler::getAvailableCores() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!running) return numCores;
    return static_cast<int>(availableCores.size());
}

void FCFSScheduler::printVMStat(){
    memoryManager->printVMStat(cpuCycles.load(), idleTicks.load(), activeTicks.load());
}

void FCFSScheduler::printProcessSMI(){
    int usedMemory = memoryManager->getUsedMemory();
    int memoryUsage = (usedMemory / max_overall_mem) * 100;

    std::stringstream oss;
    oss << "-------------------------------------------\n";
    oss << "| PROCESS-SMI V01.00 Driver Version: 01.00 |\n";
    oss << "|             PAGING ALLOCATOR             |\n";
    oss << "-------------------------------------------\n";
    oss << std::fixed << std::setprecision(2);
    oss << "CPU-Util: " << getCpuUtilization() << "%\n";
    oss << "Memory Usage: " << usedMemory << "KiB / " << max_overall_mem << "KiB\n";
    oss << "Memory Util: " << memoryUsage << "%\n";
    oss << "\n===========================================\n";
    oss << "Running processes and memory usage:\n";
    oss << "-------------------------------------------\n";
    for(auto& proc : runningProcesses) {
        std::string startAddr = "0x0040";
        int memUsage = memoryManager->getProcessMemoryUsage(proc->getName());
        oss << proc->getName() << " (" << memUsage << "KiB) \n";
    }
    oss << "-------------------------------------------\n";
    std::cout << oss.str();
}