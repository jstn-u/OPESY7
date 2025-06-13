#include "PrintCommand.h"

PrintCommand::PrintCommand(std::string& toPrint){
    this->toPrint = toPrint;
}

void PrintCommand::execute(int cpu, std::string processName, std::time_t endTime) {
    std::string processN = processName;
    std::time_t now = std::time(nullptr);
    std::tm localTime;

    localtime_s(&localTime, &now);

    std::ostringstream timeStream;
    timeStream << std::put_time(&localTime, "(%m/%d/%Y %I:%M:%S%p)");

    std::ofstream logFile(processN + ".txt", std::ios_base::app);

    if (logFile.is_open()) {
        logFile.seekp(0, std::ios::end);
        if (logFile.tellp() == 0) {
            logFile << "Process name: " << processN << std::endl;
            logFile << "Logs:" << std::endl << std::endl;
        }

        logFile << timeStream.str() << " Core:" << cpu
            << " \"" << this->toPrint << "\"" << std::endl;
        logFile.close();
    }
}

std::string PrintCommand::getToPrint() const {
    return toPrint;
}