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
        std::string getLog() const { return logEntry; }
    private:
        std::string toPrint;
        std::string logEntry;
};