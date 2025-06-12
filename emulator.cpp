#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <cstdlib>
#include "Process.h"

std::map<std::string, Process> sessions;

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

    std::string input;
    while (true) {
        std::cout << session.getName() << " > ";
        std::getline(std::cin, input);
        if (input == "exit") {
            break;
        } else {
            if (input.substr(0, 6) == "print ") {
                std::string toPrint = input.substr(6);
                std::cout << toPrint << "\n";
            }
        }
    }
    std::system("CLS");
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

                            drawScreen(sessions[sessionName]);
                            headerText();
                        } else {
                            std::cout << "No such session to resume.\n";
                        }
                    }
                } else if(option == "-ls"){
                    std::cout << "There are [" << sessions.size() << "] sessions\n";
                    std::cout << "Running Sessions:\n";
                    std::cout << std::left << std::setw(20) << "[ Session Name ]"
                            << std::right << std::setw(30) << "[ Date Created ]"
                            << std::right << std::setw(30) << "[ Status ]\n";

                    for (const auto& pair : sessions) {
                        const std::string& key = pair.first;
                        const Process& session = pair.second;

                        std::cout << std::left << std::setw(20) << session.getName()
                                << std::right << std::setw(30) << session.getTimestamp() 
                                << std::right << std::setw(27) << session.getStatus() << '\n';
                    }
                    std::cout << "\n";
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
    headerText();
    return 0;
}
