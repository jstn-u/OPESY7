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

std::map<std::string, Process> sessions;

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

void drawScreen(const Process& session) {
    std::cout << "\n=== Attached to Screen: " << session.getName() << " ===\n";
    std::cout << "Process Name         : " << session.getName() << "\n";
    std::cout << "Instruction Line     : " << session.getCurrentLine() << " / " << session.getTotalLines() << "\n";
    std::cout << "Created At           : " << session.getTimestamp() << "\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Type 'exit' to return to the main menu.\n\n";
}

void executeScreen(const Process& session){
    std::string input;
    while (true) {
        std::cout << session.getName() << " > ";
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

/*  std::cout << num_cpu;
    std::cout << scheduler;
    std::cout << quantum_cycles;
    std::cout << batch_process_freq;
    std::cout << min_ins;
    std::cout << max_ins;
    std::cout << delay_per_exec;
    std::cout << "\nConfiguration loaded from " << filename << "\n"; */
}

void executeThread(int id){
    
}

void createSampleProcesses(){
    for (int i = 0; i < 10; ++i) {
        std::string processName = std::string("process") + (i < 9 ? "0" : "") + std::to_string(i+1);
        Process* newProcess = new Process(processName, 0, 100, getCurrentTimestamp(), "Attached");
        newProcess->create100PrintCommands();
        sessions[processName] = *newProcess;
        fcfsScheduler->addProcess(newProcess);
    }
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

            if (
            command == "initialize" || 
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
                        if (sessions.find(sessionName) == sessions.end()) {
                            Process newSession = {
                                sessionName,
                                1 + rand() % 10, // mock current line
                                10 + rand() % 100, // mock total lines
                                getCurrentTimestamp(),
                                "Attached"
                            };
                            sessions[sessionName] = newSession;
                            std::system("CLS");
                            std::cout << "Session '" << sessionName << "' created.\n\n";
                            drawScreen(sessions[sessionName]);
                            executeScreen(sessions[sessionName]);
                            headerText();
                        } else {
                            std::cout << "Session already exists.\n";
                        }
                    } else if (option == "-r") {
                        if (sessions.find(sessionName) != sessions.end()) {
                            std::system("CLS");
                            Process& current = sessions[sessionName];
                            
                            if (current.getStatus() == "Detached"){
                                current.setStatus("Attached");
                            }
                            drawScreen(current);
                            executeScreen(current);
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
                                << " (" << proc->getTimestamp() << ")"
                                << "    Finished    " << proc->getTotalLines() << " / " << proc->getTotalLines() << "\n";
                    }
                    std::cout << "----------------------------------------\n";
                } else if (option == "-d") {
                    if (sessions.find(sessionName) != sessions.end()) {
                        Process& current = sessions[sessionName];
                        if (current.getStatus() == "Detached") {
                            std::cout << "Error: Session \"" << current.getName() << "\" is already detached.\n";
                        } else if (current.getStatus() == "Attached") {
                            current.setStatus("Detached");
                            std::cout << current.getName() << " has been detached successfully.\n";
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
    fcfsScheduler = new FCFSScheduler(4); // 4 cores
    createSampleProcesses();
    fcfsScheduler->start();
    loadConfig("config.txt");
    headerText();
    fcfsScheduler->stop();
    delete fcfsScheduler;
    return 0;
}