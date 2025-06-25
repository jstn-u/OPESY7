#include "PrintCommand.h"

PrintCommand::PrintCommand(const std::string& toPrint){
    this->toPrint = toPrint;
}

void PrintCommand::execute(int cpu, std::string processName, std::time_t endTime) {
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &endTime);
#else
    localtime_r(&endTime, &localTime);
#endif
    std::ostringstream timeStream;
    timeStream << std::put_time(&localTime, "(%m/%d/%Y %I:%M:%S%p)");
    logEntry = timeStream.str() + "    Core:" + std::to_string(cpu) + "    \"" + this->toPrint + "\"";
}

/* void PrintCommand::execute(int cpu, std::string processName, std::time_t endTime) {
    std::time_t now = std::time(nullptr);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    std::ostringstream timeStream;
    timeStream << std::put_time(&localTime, "(%m/%d/%Y %I:%M:%S%p)");

    std::ofstream logFile(processName + ".txt", std::ios_base::app);

    if (logFile.is_open()) {
        logFile.seekp(0, std::ios::end);
        if (logFile.tellp() == 0) {
            logFile << "Process name: " << processName << std::endl;
            logFile << "Logs:" << std::endl;
        }
        logFile << timeStream.str() << "    Core:" << cpu
            << "    \"" << this->toPrint << "\"" << std::endl;
        logFile.close();
    }
} */