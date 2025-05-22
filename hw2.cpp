#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

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
        "-'screen'\n"
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
            
            /* Use this once actual functions are implemented
            if (command == "initialize"){
                std::cout << "initialize command recognized. Doing something.\n";
            }
            else if (command == "scheduler-test") {
                std::cout << "scheduler-test command recognized. Doing something.\n";
            }
            else if (command == "scheduler-stop") {
                std::cout << "scheduler-stop command recognized. Doing something.\n";
            }
            else if (command == "report-util") {
                std::cout << "report-util command recognized. Doing something.\n";
            }
            */

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
                
                if (option == "-r") {
                    std::cout << "Reattaching to a screen session...\n";
                    // Insert reattach logic here
                } 
                else if (option == "-s") {
                    std::string sessionName;
                    iss >> sessionName; // optionally read third word
                    if (!sessionName.empty()) {
                        std::cout << sessionName;
                    }
                }
                else {
                    std::cout << "Unknown screen option: " << option << "\n";
                }
            }
            else if (command == "clear") {
                std:: system("CLS");
                //system("cls"); <- for windows
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
    headerText();
    return 0;
}