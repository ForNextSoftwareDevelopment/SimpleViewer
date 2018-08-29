#pragma once
#include <string>
#include <vector>
#include <cstdio>

#define DEBUG

namespace Error
{
    // Application folder
    static std::string logFilePath;

    // Messages to log / display
    static std::vector<std::string> info;

    // Set logfile location
    void SetLogFilePath(std::string path);

    // Get logfile location
    std::string GetLogFilePath(void);

    // Create new logfile
    void CreateLog (void);

    // Write in logfile
    void WriteLog (const char *type, const char *className, const char *message);
};

