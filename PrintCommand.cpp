#include "PrintCommand.h"
#include "Process.h"

static const uint16_t INVALID_VAL = 0xFFFF;

PrintCommand::PrintCommand(const std::string& toPrint) {
    this->toPrint = toPrint;
}

PrintCommand::PrintCommand(int type, std::string decVar, std::uint16_t decVal, std::string sumOrDiffVar, std::uint16_t val1, std::uint16_t val2, std::string var1, std::string var2, std::string writeMemAdd, std::string writeVar, std::uint16_t writeVal, std::string readVar, std::string readMemAdd, std::string message, std::string print)
{
    this->type = type;

    this->decVar = decVar;
    this->decVal = decVal;

    this->sumOrDiffVar = sumOrDiffVar;
    this->val1 = val1;
    this->val2 = val2;
    this->var1 = var1;
    this->var2 = var2;

    this->writeMemAdd = writeMemAdd;
    this->writeVar = writeVar;
    this->writeVal = writeVal;

    this->readVar = readVar;
    this->readMemAdd = readMemAdd;

    this->message = message;
    this->print = print;
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

void PrintCommand::execute2(Process& process)
{
    if (type == 1) {
        process.declaredVarss[decVar] = decVal;
    }

    if (type == 2) {
        uint16_t val = process.declaredVarss[writeVar];
        process.memoryAddSpace[writeMemAdd] = val;
    }

    if (type == 3) {
        uint16_t val = process.memoryAddSpace[readMemAdd];
        process.declaredVarss[readVar] = val;
    }

    if (type == 4) { // ADD
        if (val1 == INVALID_VAL && val2 == INVALID_VAL && var1 != "" && var2 != "") {
            process.declaredVarss[sumOrDiffVar] = process.declaredVarss[var1] + process.declaredVarss[var2];
        }
        else if (val1 == INVALID_VAL && var1 != "" && val2 != INVALID_VAL && var2 == "") {
            process.declaredVarss[sumOrDiffVar] = process.declaredVarss[var1] + val2;
        }
        else if (val2 == INVALID_VAL && var2 != "" && val1 != INVALID_VAL && var1 == "") {
            process.declaredVarss[sumOrDiffVar] = val1 + process.declaredVarss[var2];
        }
        else {
            process.declaredVarss[sumOrDiffVar] = val1 + val2;
        }
    }

    if (type == 5) { // SUB
        if (val1 == INVALID_VAL && val2 == INVALID_VAL && var1 != "" && var2 != "") {
            process.declaredVarss[sumOrDiffVar] = process.declaredVarss[var1] - process.declaredVarss[var2];
        }
        else if (val1 == INVALID_VAL && var1 != "" && val2 != INVALID_VAL && var2 == "") {
            process.declaredVarss[sumOrDiffVar] = process.declaredVarss[var1] - val2;
        }
        else if (val2 == INVALID_VAL && var2 != "" && val1 != INVALID_VAL && var1 == "") {
            process.declaredVarss[sumOrDiffVar] = val1 - process.declaredVarss[var2];
        }
        else {
            process.declaredVarss[sumOrDiffVar] = val1 - val2;
        }
    }

    if (type == 6) { // PRINT
        std::string printout = message + std::to_string(process.declaredVarss[print]);
        process.logPrint(printout);
    }

    if (type == 7) {
        // SLEEP logic (to be implemented)
    }
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