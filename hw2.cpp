#include <iostream>
#include <string>
#include <cstdlib>

void headerText ()
{
    std::cout<< " ----------      ----------      ----------      ----------      -----------     ----------      ---    --- \n";
    std::cout<< "|          |    |          |    |          |    |          |    |           |   |          |    |   |  |   |\n";
    std::cout<< "|    ------     |    ------     |    --    |    |    --    |    |   --------    |    ------     |   |  |   |\n";
    std::cout<< "|   |           |   |           |   |  |   |    |   |  |   |    |  |            |   |           |   |  |   |\n";
    std::cout<< "|   |           |    ------     |   |  |   |    |    --    |    |   --------    |    ------     |    --    |\n";
    std::cout<< "|   |           |          |    |   |  |   |    |          |                |   |          |    |          |\n";
    std::cout<< "|   |            ------    |    |   |  |   |    |    ------     |   --------     ------    |     ------    |\n";
    std::cout<< "|   |                  |   |    |   |  |   |    |   |           |  |                   |   |           |   |\n";
    std::cout<< "|    ------      ------    |    |    --    |    |   |           |   --------     ------    |     ------    |\n";
    std::cout<< "|          |    |          |    |          |    |   |           |           |   |          |    |          |\n";
    std::cout<< " ----------      ----------      ----------      ---             -----------     ----------      ---------- \n";
    std::cout<< "\n";
    std::cout<< "Hello, Welcome to the CSOPESY command line emulator!\n";
    std::cout<< "Type:\n";
    std::cout<< "-'initialize'\n";
    std::cout<< "-'screen'\n";
    std::cout<< "-'scheduler-test'\n";
    std::cout<< "-'scheduler-stop'\n";
    std::cout<< "-'report-util'\n";
    std::cout<< "-'clear' to clear the screen\n";
    std::cout<< "-'exit' to quit\n";
    std::cout<< "\n";

    std:: string command;
    std::cout<< "Enter a command: ";
    std:: cin >> command;

    
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
    else if (command == "clear") {
        std:: system("clear");
        //system("cls"); <- for windows
        std::cout << "clear command recognized. The screen will clear out and return to the header.\n";
        std::cout << "\n";
        headerText();
    }
    else if (command == "exit") {
        std::cout << "exit command recognized. exiting...\n";
        exit(0);
    }
    else {
        std::cout << "Not a valid command. Try again\n";
        headerText();
    }
}

int main()
{
    headerText();
    return 0;
}