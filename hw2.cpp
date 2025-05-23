#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <cstdlib>

// Struct to store session information
struct ScreenSession {
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;
};

std::map<std::string, ScreenSession> sessions;

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

void drawScreen(const ScreenSession& session) {
    std::cout << "\n=== Attached to Screen: " << session.name << " ===\n";
    std::cout << "Process Name         : " << session.name << "\n";
    std::cout << "Instruction Line     : " << session.currentLine << " / " << session.totalLines << "\n";
    std::cout << "Created At           : " << session.timestamp << "\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Type 'exit' to return to the main menu.\n\n";

    std::string input;
    while (true) {
        std::cout << session.name << " > ";
        std::getline(std::cin, input);
        if (input == "exit") {
            break;
        } else {
            std::cout << "[Session running] You typed: " << input << "\n";
        }
    }
    std::system("CLS");
}

void drawProcess(){
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
                            ScreenSession newSession = {
                                sessionName,
                                1 + rand() % 10, // mock current line
                                10 + rand() % 100, // mock total lines
                                getCurrentTimestamp()
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
                        std::system("CLS");
                        if (sessions.find(sessionName) != sessions.end()) {
                            drawScreen(sessions[sessionName]);
                            headerText();
                        } else {
                            std::cout << "No such session to resume.\n";
                        }
                    }
                } else if(option == "-ls"){
                    std::cout << "There are [" << sessions.size() << "] sessions" << std::endl;
                    std::cout << "Running Sessions:\n";
                    std::cout << std::left << std::setw(20) << "[ Session Name ]"
                            << std::right << std::setw(30) << "[ Date Created ]\n";

                    for (const auto& pair : sessions) {
                        const std::string& key = pair.first;
                        const ScreenSession& session = pair.second;

                        std::cout << std::left << std::setw(20) << session.name
                                << std::right << std::setw(30) << session.timestamp << '\n';
                    }
                    std::cout << "\n";
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
