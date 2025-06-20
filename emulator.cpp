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
#include <queue>
#include <condition_variable>
#include <atomic>
#include <algorithm>

int num_cpu;
std::string scheduler;
int quantum_cycles;
int batch_process_freq;
int min_ins;
int max_ins;
int delay_per_exec;
FCFSScheduler* fcfsScheduler = nullptr;

std::map<std::string, Process*> readyProcesses;
std::map<std::string, Process*> runningProcessesMap;
std::map<std::string, Process*> finishedProcessesMap;

std::queue<Process*> readyQueue;
std::mutex queueMutex;
std::condition_variable cv;
std::atomic<bool> schedulerRunning{true};

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

void executeScreen(const Process* session){
    std::string input;
    while (true) {
        std::cout << session->getName() << " > ";
        std::getline(std::cin, input);
        if (input == "exit") {
            break;
        } else {
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

void createSampleProcesses(){
    for (int i = 0; i < 10; ++i) {
        std::string processName = std::string("process") + (i < 9 ? "0" : "") + std::to_string(i+1);
        Process* newProcess = new Process(processName, 0, 100, getCurrentTimestamp(), "Attached");
        newProcess->create100PrintCommands();
        fcfsScheduler->addProcess(newProcess);
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
            else if (
            command == "scheduler-test" || 
            command == "scheduler-stop" || 
            command == "report-util") {
                std::cout << command + " command recognized. Doing something.\n";
            }
            else if (command == "screen") {
                std::string option;
                iss >> option;  // Read the second word after "screen"
                std::string sessionName;
                iss >> sessionName;

                if ((option == "-r" || option == "-s") && !sessionName.empty()) {
                    if (option == "-s") {
                        Process *proc = findProcess(sessionName);
                        if (proc == nullptr) {
                            Process* newSession = new Process(
                                sessionName,
                                0,
                                100,
                                getCurrentTimestamp(),
                                "Attached"
                            );

                            // Add to scheduler
                            if(scheduler == "fcfs"){
                                fcfsScheduler->addProcess(newSession);
                            }
                            std::system("CLS");
                            std::cout << "Session '" << sessionName << "' created.\n\n";
                            drawScreen(sessionName);
                            executeScreen(newSession);
                            headerText();
                        } else {
                            std::cout << "Session already exists.\n";
                        }
                    } else if (option == "-r") {
                        Process *proc = findProcess(sessionName);
                        if (proc != nullptr) {
                            std::system("CLS");
                            if (proc->getStatus() == "Detached"){
                                proc->setStatus("Attached");
                            }
                            drawScreen(sessionName);
                            executeScreen(proc);
                            headerText();
                        } else {
                            std::cout << "No such session to resume.\n";
                        }
                    }
                } else if(option == "-ls"){
                    auto running = fcfsScheduler->getRunningProcesses();
                    auto finished = fcfsScheduler->getFinishedProcesses();

                    // Sort running processes by name
                    std::sort(running.begin(), running.end(), [](Process* a, Process* b) {
                        return a->getName() < b->getName();
                    });

                    // Sort finished processes by name
                    std::sort(finished.begin(), finished.end(), [](Process* a, Process* b) {
                        return a->getName() < b->getName();
                    });
                    std::cout << "----------------------------------------\n";
                    std::cout << "Running processes:\n";
                    for (auto* proc : running) {
                        std::cout << std::left << std::setw(12) << proc->getName()
                                << " (" << proc->getTimestamp() << ")"
                                << "    Core: " << proc->getCpuId()
                                << "    " << proc->getCurrentLine() << " / " << proc->getTotalLines() << "\n";
                    }
                    std::cout << "\nFinished processes:\n";
                    for (auto* proc : finished) {
                        std::cout << std::left << std::setw(12) << proc->getName()
                                << " (" << proc->getEndTime() << ")"
                                << "    Finished    " << proc->getTotalLines() << " / " << proc->getTotalLines() << "\n";
                    }
                    std::cout << "----------------------------------------\n";
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
            else if (command == "clear") {
                std::system("CLS");
                break;
            }
            else if (command == "exit") {
                std::cout << "exit command recognized. exiting...\n";
                exit(0);
            }
            else {
                std::cout << "Not a valid command. Try again\n";
            }
        }
    }
}

int main() {
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
                //fcfsScheduler->start();
                break;
            } else {
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