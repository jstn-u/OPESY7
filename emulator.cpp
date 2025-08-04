#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <cstdlib>
#include <thread>
#include <vector>
#include <fstream>
#include <mutex>
#include "Process.h"
#include "PrintCommand.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"
#include "MemoryManager.h"
#include <queue>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <random> // Include for random number generation

// Utility function to generate a random integer in [min, max]
int randInt(int min, int max) {
    return min + (std::rand() % (max - min + 1));
}

int num_cpu;
std::string scheduler;
int quantum_cycles;
int batch_process_freq;
int min_ins;
int max_ins;
int delay_per_exec;
FCFSScheduler* fcfsScheduler = nullptr;
RRScheduler* rrScheduler = nullptr;
int curr_id = 0;
int max_overall_mem;
int mem_per_frame;
int min_mem_per_proc;
int max_mem_per_proc;

std::map<std::string, Process*> readyProcesses;
std::map<std::string, Process*> runningProcessesMap;
std::map<std::string, Process*> finishedProcessesMap;

std::queue<Process*> readyQueue;
std::mutex queueMutex;
std::condition_variable cv;
std::atomic<bool> schedulerRunning{true};
std::atomic<bool> schedulerActive{false};

std::stringstream outputBuffer;

std::string getCurrentTimestamp() {
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

void updateProcessMaps() {
    // Clear and repopulate maps from scheduler's vectors
    runningProcessesMap.clear();
    finishedProcessesMap.clear();
    readyProcesses.clear();

    if(fcfsScheduler){
        auto running = fcfsScheduler->getRunningProcesses();
        auto finished = fcfsScheduler->getFinishedProcesses();
        auto ready = fcfsScheduler->getReadyProcesses();
        for (auto* proc : running) {
            runningProcessesMap[proc->getName()] = proc;
        }
        for (auto* proc : finished) {
            finishedProcessesMap[proc->getName()] = proc;
        }
        for (auto* proc : ready) {
            readyProcesses[proc->getName()] = proc;
        }
    }else if(rrScheduler) {
        auto running = rrScheduler->getRunningProcesses();
        auto finished = rrScheduler->getFinishedProcesses();
        auto ready = rrScheduler->getReadyProcesses();
        for (auto* proc : running) {
            runningProcessesMap[proc->getName()] = proc;
        }
        for (auto* proc : finished) {
            finishedProcessesMap[proc->getName()] = proc;
        }
        for (auto* proc : ready) {
            readyProcesses[proc->getName()] = proc;
        }
    }

}

Process* findProcess(const std::string& processName) {
    updateProcessMaps();
    Process* proc = nullptr;
    // Check running first, then finished
    if (runningProcessesMap.count(processName)) {
        proc = runningProcessesMap[processName];
    } else if (finishedProcessesMap.count(processName)) {
        proc = finishedProcessesMap[processName];
    }else if(readyProcesses.count(processName)) {
        proc = readyProcesses[processName];
    }

    if (!proc) {
        return nullptr;
    }
    
    return proc;
}

void drawScreen(const std::string& processName) {
    Process* proc = findProcess(processName);
    
    if(!proc) {
        std::cout << "Error: Process not found.\n";
        return;
    }

    std::cout << "\n=== Attached to Screen: " << proc->getName() << " ===\n";
    std::cout << "Process Name         : " << proc->getName() << "\n";
    int currentLineDisplay = proc->getStatus() == "Finished" ? proc->getTotalLines() : proc->getCurrentLine();
    std::cout << "Instruction Line     : " << currentLineDisplay << " / " << proc->getTotalLines() << "\n";
    std::cout << "Created At           : " << proc->getTimestamp() << "\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Type 'exit' to return to the main menu.\n\n";
}

void executeScreen(const std::string& processName){
    Process* proc = findProcess(processName);
    std::string input;
    
    while (true) {
        std::cout << proc->getName() << " > ";
        std::getline(std::cin, input);
        if (input == "exit") {
            break;
        } 
        else if(input == "process-smi"){
            std::cout << "Process name: " << proc->getName() << "\n";
            std::cout << "ID: " << proc->getPid() << "\n";
            std::cout << "Logs: \n";
            std::vector<std::string> logs = proc->getAllLogs();
            for(const auto& log : logs) {
                std::cout << log << "\n";
            }
        }
        else {
            if (input.substr(0, 6) == "PRINT(" && input.back() == ')') {
                std::string toPrint = input.substr(6, input.length() - 7);
                std::cout << toPrint << "\n";
            }
        }
    }
    std::system("CLS");
}

void loadConfig(const std::string& filename) {
    
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string key;
        if (iss >> key) {
            if (key == "num-cpu") iss >> num_cpu;
            else if (key == "scheduler") iss >> std::quoted(scheduler);
            else if (key == "quantum-cycles") iss >> quantum_cycles;
            else if (key == "batch-process-freq") iss >> batch_process_freq;
            else if (key == "min-ins") iss >> min_ins;
            else if (key == "max-ins") iss >> max_ins;
            else if (key == "delay-per-exec") iss >> delay_per_exec;
            else if (key == "max-overall-mem") iss >> max_overall_mem;
            else if (key == "mem-per-frame") iss >> mem_per_frame;
            else if (key == "min-mem-per-proc") iss >> min_mem_per_proc;
            else if (key == "max-mem-per-proc") iss >> max_mem_per_proc;
        }
    }

    /* std::cout << num_cpu;
    std::cout << scheduler;
    std::cout << quantum_cycles;
    std::cout << batch_process_freq;
    std::cout << min_ins;
    std::cout << max_ins;
    std::cout << delay_per_exec;
    std::cout << "\nConfiguration loaded from " << filename << "\n"; */
}

/* void processGeneratorFunc() {
    while (schedulerActive) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate a CPU tick (adjust as needed)
        int tick = ++cpuTick;
        if (batch_process_freq > 0 && tick % batch_process_freq == 0) {
            std::string processName = "auto_proc_" + std::to_string(tick);
            Process* newProcess = new Process(processName, 0, 100, getCurrentTimestamp(), "Ready");
            newProcess->create100PrintCommands();
            fcfsScheduler->addProcess(newProcess);
        }
    }
} */

/* void createSampleProcesses(){
    for (int i = 0; i < 10; ++i) {
        std::string processName = std::string("process") + (i < 9 ? "0" : "") + std::to_string(i+1);
        Process* newProcess = new Process(curr_id, processName, 0, 100, getCurrentTimestamp(), "Attached");
        ++curr_id;
        newProcess->createPrintCommands(100);
        fcfsScheduler->addProcess(newProcess);
    }
} */

float getCpuUtilization() {
    if (scheduler == "fcfs") {
        return fcfsScheduler->getCpuUtilization();
    } else if (scheduler == "rr") {
        return rrScheduler->getCpuUtilization();
    }
    return 0.0f;
}

int getBusyCores() {
    if (scheduler == "fcfs") {
        return fcfsScheduler->getBusyCores();
    } else if (scheduler == "rr") {
        return rrScheduler->getBusyCores();
    }
    return 0;
}

int getAvailableCores() {
    if (scheduler == "fcfs") {
        return fcfsScheduler->getAvailableCores();
    } else if (scheduler == "rr") {
        return rrScheduler->getAvailableCores();
    }
    return 0;
}

void screenLS(std::vector<Process*> running, std::vector<Process*> finished) {
    // Sort running processes by name
    std::sort(running.begin(), running.end(), [](Process* a, Process* b) {
        return a->getName() < b->getName();
    });

    // Sort finished processes by end time (earliest first)
    std::sort(finished.begin(), finished.end(), [](Process* a, Process* b) {
        return a->getEndTime() < b->getEndTime();
    });
    outputBuffer.str(""); // Clear the output buffer

    outputBuffer << "CPU Utilization: " << getCpuUtilization() << "%" << "\n";
    outputBuffer << "Cores Used: " << getBusyCores() << "\n";
    outputBuffer << "Cores Available: " << getAvailableCores() << "\n";

    outputBuffer << "----------------------------------------\n";
    outputBuffer << "Running processes:\n";
    for (auto* proc : running) {
        outputBuffer << std::left << std::setw(12) << proc->getName()
                << " (" << proc->getTimestamp() << ")"
                << "    Core: " << proc->getCpuId()
                << "    " << proc->getCurrentLine() << " / " << proc->getTotalLines() << "\n";
    }
    outputBuffer << "\nFinished processes:\n";
    for (auto* proc : finished) {
        std::string status = proc->getStatus();
        if (status.find("Terminated") != std::string::npos) {
            outputBuffer << std::left << std::setw(12) << proc->getName()
                << " (" << proc->getEndTime() << ")"
                << "    Terminated    " << proc->getCurrentLine() << " / " << proc->getTotalLines() << "\n";
        } else {
            outputBuffer << std::left << std::setw(12) << proc->getName()
                << " (" << proc->getEndTime() << ")"
                << "    Finished    " << proc->getTotalLines() << " / " << proc->getTotalLines() << "\n";
        }
    }
    outputBuffer << "----------------------------------------\n";
}

void reportUtil(){
    if(scheduler == "fcfs"){
        std::vector<Process*> running = fcfsScheduler->getRunningProcesses();
        std::vector<Process*> finished = fcfsScheduler->getFinishedProcesses();
        screenLS(running, finished);
    }else if(scheduler == "rr"){
        std::vector<Process*> running = rrScheduler->getRunningProcesses();
        std::vector<Process*> finished = rrScheduler->getFinishedProcesses();
        screenLS(running, finished);
    }

    std::ofstream reportFile("csopesy-log.txt");
    if (reportFile.is_open()) {
        reportFile << outputBuffer.str();
        reportFile.close();
        std::cout << "Report saved to csopesy-log.txt\n";
    } else {
        std::cerr << "Error opening file!" << std::endl;
    }
}

void initScreen(){
    std::cout<< R"(
   _____  _____  ____  _____  ______  _______     __
  / ____|/ ____|/ __ \|  __ \|  ____|/ ____\ \   / /
 | |    | (___ | |  | | |__) | |__  | (___  \ \_/ / 
 | |     \___ \| |  | |  ___/|  __|  \___ \  \   /  
 | |____ ____) | |__| | |    | |____ ____) |  | |   
  \_____|_____/ \____/|_|    |______|_____/   |_|                             
    )";
        std::cout<< "\n"
        "Hello, Welcome to the CSOPESY command line emulator!\n"
        "Type initialize to start the emulator.\n";
}

void headerText () {
    while(true) {
        std::cout<< R"(
   _____  _____  ____  _____  ______  _______     __
  / ____|/ ____|/ __ \|  __ \|  ____|/ ____\ \   / /
 | |    | (___ | |  | | |__) | |__  | (___  \ \_/ / 
 | |     \___ \| |  | |  ___/|  __|  \___ \  \   /  
 | |____ ____) | |__| | |    | |____ ____) |  | |   
  \_____|_____/ \____/|_|    |______|_____/   |_|                             
    )";
        std::cout<< "\n"
        "Hello, Welcome to the CSOPESY command line emulator!\n"
        "Type:\n"
        "-'initialize'\n"
        "-'screen -s <name>' to create a session\n"
        "-'screen -r <name>' to resume a session\n"
        "-'screen -ls' to view all attached or detached sessions\n"
        "-'screen -d <name>' to detach a running session\n"
        "-'scheduler-test'\n"
        "-'scheduler-stop'\n"
        "-'report-util'\n"
        "-'clear' to clear the screen\n"
        "-'exit' to quit\n"
        "\n";

        std::cout << min_mem_per_proc << " " << max_mem_per_proc << "\n";

        while(true) {
            std:: string input;
            std:: cout<< "Enter a command: ";
            std::getline(std::cin, input); 
            std::istringstream iss(input);
            std:: string command;
            iss >> command;

            if(command == "initialize") {
                std::cout << "Emulator has already been initialized.\n";
            }
            else if (command == "report-util") {
                updateProcessMaps();
                reportUtil();
            }
            else if (command == "screen") {
                std::string option;
                iss >> option;  // Read the second word after "screen"
                std::string sessionName;
                iss >> sessionName;

                if ((option == "-r" || option == "-s" || option == "-c") && !sessionName.empty()) {
                    if (option == "-s") {
                        std::string procSize;
                        iss >> procSize;
                        int memSize = std::stoi(procSize);

                        if((memSize & (memSize - 1)) == 0){
                            std::cout << "Memory is valid.\n";

                            Process *proc = findProcess(sessionName);
                            if (proc == nullptr) {
                                int numInstructions = min_ins + (std::rand() % (max_ins - min_ins + 1));

                                Process* newSession = new Process(
                                    curr_id,
                                    sessionName,
                                    0,
                                    numInstructions,
                                    getCurrentTimestamp(),
                                    "Ready",
                                    memSize
                                );

                                newSession->createPrintCommands(numInstructions);
                                ++curr_id;
                                // Add to scheduler
                                if(scheduler == "fcfs"){
                                    fcfsScheduler->addProcess(newSession);
                                }else if(scheduler == "rr"){
                                    rrScheduler->addProcess(newSession);
                                }
                                std::system("CLS");
                                std::cout << "Session '" << sessionName << "' created.\n\n";
                                drawScreen(sessionName);
                                executeScreen(sessionName);
                                headerText();
                                
                            } else {
                                std::cout << "Session already exists.\n";
                            }
                        }else{
                            std::cout << "Invalid memory size. Must be a power of 2.\n";
                        }
                    } else if (option == "-r") {
                        Process *proc = findProcess(sessionName);
                        if (proc != nullptr) {
                            std::system("CLS");
                            if (proc->getStatus() == "Detached"){
                                proc->setStatus("Attached");
                            }
                            drawScreen(sessionName);
                            executeScreen(sessionName);
                            headerText();
                        } else {
                            std::cout << "No such session to resume.\n";
                        }
                    } else if (option == "-c"){
                        
                    }
                } else if(option == "-ls"){
                    if(scheduler == "fcfs"){
                        std::vector<Process*> running = fcfsScheduler->getRunningProcesses();
                        std::vector<Process*> finished = fcfsScheduler->getFinishedProcesses();
                        screenLS(running, finished);
                        std::cout << outputBuffer.str();
                    }
                    else if(scheduler == "rr"){
                        updateProcessMaps();
                        std::vector<Process*> running = rrScheduler->getRunningProcesses();
                        std::vector<Process*> finished = rrScheduler->getFinishedProcesses();
                        screenLS(running, finished);
                        std::cout << outputBuffer.str();
                    }
                } else if (option == "-d") {
                    Process *current = findProcess(sessionName);
                    if (current != nullptr) {
                        //Process& current = sessions[sessionName];
                        if (current->getStatus() == "Detached") {
                            std::cout << "Error: Session \"" << current->getName() << "\" is already detached.\n";
                        } else if (current->getStatus() == "Attached") {
                            current->setStatus("Detached");
                            std::cout << current->getName() << " has been detached successfully.\n";
                        } else {
                            std::cout << "Error: Unknown session status.\n";
                        }
                    } else {
                        std::cout << "Error: Session \"" << sessionName << "\" does not exist.\n";
                    }
                }
                else {
                    std::cout << "Invalid screen command format.\n";
                }
            }
            else if(command == "scheduler-start") {
                if (fcfsScheduler && !fcfsScheduler->isRunning()) {
                    fcfsScheduler->start();
                    fcfsScheduler->startProcessGenerator(batch_process_freq);
                    std::cout << "FCFS Scheduler started.\n";
                } else if(rrScheduler && !rrScheduler->isRunning()) {
                    rrScheduler->start();
                    rrScheduler->startProcessGenerator(batch_process_freq);
                    std::cout << "RR Scheduler started.\n";
                }
                else {
                    std::cout << "Scheduler is already running.\n";
                }
            }
            else if(command == "scheduler-stop") {
                if (fcfsScheduler && fcfsScheduler->isRunning()) {
                    fcfsScheduler->stopProcessGenerator();
                    std::cout << "Process generator stopped. All existing processes will be finished.\n";
                } else if(rrScheduler && rrScheduler->isRunning()) {
                    rrScheduler->stopProcessGenerator();
                    std::cout << "Process generator stopped. All existing processes will be finished.\n";
                }
                else {
                    std::cout << "Scheduler is not running.\n";
                }
            }
            else if (command == "clear") {
                std::system("CLS");
                break;
            }
            else if (command == "exit") {
                std::cout << "exit command recognized. exiting...\n";
                fcfsScheduler->stop();
                rrScheduler->stop();
                delete fcfsScheduler;
                delete rrScheduler;
                exit(0);
            }
            else if (command == "vmstat"){
                if(rrScheduler && rrScheduler->isRunning()){
                    rrScheduler->printVMStat();
                }
            }
            else if (command == "process-smi"){
                if(rrScheduler && rrScheduler->isRunning()){
                    rrScheduler->printProcessSMI();
                } else {
                    std::cout << "No scheduler is running.\n";
                }
            }
            else {
                std::cout << "Not a valid command. Try again\n";
            }
        }
    }
}

// When a process is created and added, immediately notify all worker threads to wake up and execute
void addAndRunProcess(Process* proc) {
    if (scheduler == "fcfs" && fcfsScheduler) {
        fcfsScheduler->addProcess(proc);
        // No explicit notify needed, FCFS addProcess already notifies
    } else if (scheduler == "rr" && rrScheduler) {
        rrScheduler->addProcess(proc);
        // No explicit notify needed, RR addProcess already notifies
    }
    // The scheduler's addProcess already calls cv.notify_all(), so workers will wake up immediately
}

std::mt19937 rng; // Define the random number generator

int main() {
    rng.seed(static_cast<unsigned int>(std::time(nullptr))); // Seed once
    std::system("CLS");
    bool init = false;
    std::string in;
    initScreen();

    while(!init){
        std::cin >> in;
        if(in == "initialize") {
            loadConfig("config.txt");
            in = true;

            if(scheduler == "fcfs") {
                fcfsScheduler = new FCFSScheduler(num_cpu);
                break;
            }else if(scheduler == "rr"){
                rrScheduler = new RRScheduler(num_cpu, quantum_cycles);
                break;
            } 
            else {
                std::cout << "Error: Unsupported scheduler type.\n";
                break;
            }
        }else if(in == "exit") {
            std::cout << "Exiting emulator...\n";
            return 0;
        } else if(in == "clear") {
            std::system("CLS");
            initScreen();
        }
        else{
            std::cout << "Invalid command\n";
        }
    }
    
    std::system("CLS");
    headerText();

    /* Week 6 HW Commands */
    /* fcfsScheduler = new FCFSScheduler(4); // 4 cores
    createSampleProcesses();
    fcfsScheduler->start();
    fcfsScheduler->stop();
    delete fcfsScheduler; */
    return 0;
}