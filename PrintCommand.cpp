#include "PrintCommand.h"
#include <string>
#include <sstream>
#include <iostream>

PrintCommand::PrintCommand(std::string& toPrint) : ICommand(PRINT){
    this->toPrint = toPrint;
}

void PrintCommand::execute(){
    std::cout << "Print Command: " << toPrint << "\n";
}

std::string PrintCommand::getToPrint() const {
    return toPrint;
}