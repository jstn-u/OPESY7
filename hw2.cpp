#include <iostream>
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
            std:: string command;
            std::cout<< "Enter a command: ";
            std:: cin >> command;
            
            /* Use this once actual functions are implemented
            if (command == "initialize"){
                std::cout << "initialize command recognized. Doing something.\n";
            }
            else if (command == "screen") {
                std::cout << "screen command recognized. Doing something.\n";
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
            command == "screen" || 
            command == "scheduler-test" || 
            command == "scheduler-stop" || 
            command == "report-util") {
                std::cout << command + " command recognized. Doing something.\n";
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