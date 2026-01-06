#pragma once
#include <windows.h>
#include <fstream>
#include <string>

class Logger {
public:
    static void Log(const std::string& message) {
        std::ofstream logFile("F4MP.log", std::ios_base::app);
        if (logFile.is_open()) {
            logFile << "[LOG] " << message << std::endl;
        }
    }
};