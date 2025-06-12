#pragma once
#include <string>
#include "ICommand.h"

class PrintCommand : public ICommand{
    public:
        PrintCommand(std::string& toPrint);
        void execute() override;
    private:
        std::string toPrint;
};