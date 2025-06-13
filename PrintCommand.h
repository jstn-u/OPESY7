#pragma once
#include <string>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>

class PrintCommand{
    public:
        PrintCommand(const std::string& toPrint);
        void execute(int cpuId, std::string processName, std::time_t endTime);
        std::string getToPrint() const;
    private:
        std::string toPrint;
};