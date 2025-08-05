#pragma once
#include <string>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <cstdint>
#include <string>
#include <iostream>

class Process;

class PrintCommand {
public:
    PrintCommand(const std::string& toPrint);
	PrintCommand(int type, std::string decVar, std::uint16_t decVal, 
        std::string sumOrDiffVar, std::uint16_t val1, std::uint16_t val2, 
        std::string var1, std::string var2, std::string writeMemAdd, 
        std::string writeVar, std::uint16_t writeVal, std::string readVar, 
        std::string readMemAdd, std::string message, std::string print);
        
    std::string getLog() const { return logEntry; }
    std::string getInstruction() const { return toPrint; }
    void execute(int cpuId, std::string processName, std::time_t endTime);
    void execute2(Process& process);
    std::string getToPrint() const;
private:
    std::string toPrint;
    std::string logEntry;
    
	int type;
	// DECLARE
	std::string decVar = "";
	std::uint16_t decVal = 0;

	// ADD // SUB
	std::string sumOrDiffVar = "";
	std::uint16_t val1 = 0;
	std::uint16_t val2 = 0;
	std::string var1= "";
	std::string var2 = "";

	// WRITE
	std::string writeMemAdd = "";
	std::string writeVar = "";
	std::uint16_t writeVal = 0;

	// READ
	std::string readVar = "";
	std::string readMemAdd = "";

	// PRINT
	std::string message = "";
	std::string print = "";
};