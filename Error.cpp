#include "Error.h"

namespace Error
{
    /*********************************************************************
    * Set logfile location
    *********************************************************************/
    void SetLogFilePath (std::string path)
    {
        logFilePath.append(path);
    };

    /*********************************************************************
    * Get logfile location
    *********************************************************************/
    std::string GetLogFilePath(void)
    {
        return (logFilePath);
    };

    /*********************************************************************
    * Create new logfile
    *********************************************************************/
    void CreateLog (void)
    {
        // Open debug log file
        FILE *logFile = NULL;
        time_t t;
        std::string strTime;
        std::string strMessage;

        logFile = fopen((logFilePath + "Log.txt").c_str() , "w" );
        if (logFile != NULL)
        {
           // Write info message to log file
            time (&t);
            strTime = ctime(&t);

            // Remove last '\r' from the string (returned from ctime)
            strTime[strTime.length() - 1] = ' ';

            strMessage.append("------------------------------INFO------------------------------\r\n\r\n");
            strMessage.append("TIMESTAMP: ");
            strMessage.append(strTime);
            strMessage.append("\r\n");
            strMessage.append("NEW LOG FILE CREATED");
            strMessage.append("\r\n----------------------------------------------------------------\r\n\r\n");
            fputs(strMessage.c_str(), logFile);

            #ifdef DEBUG
                fprintf(stderr, "%s", strMessage.c_str());
            #endif // DEBUG

            // Close file
            fclose(logFile);
        };
    };

    /*********************************************************************
    * Write in logfile
    *********************************************************************/
    void WriteLog (const char *type, const char *className, const char *message)
    {
        // Open debug log file
        FILE *logFile = NULL;
        time_t t;
        std::string strTime;
        std::string strMessage;

        logFile = fopen((logFilePath + "Log.txt").c_str() , "a" );
        if (logFile != NULL)
        {
            // Write info message to log file
            time (&t);
            strTime = ctime(&t);

            // Remove last '\r' from the string (returned from ctime)
            strTime[strTime.length() - 1] = ' ';

            strMessage.append("------------------------------");
            strMessage.append(type);
            strMessage.append("------------------------------\r\n\r\n");
            strMessage.append("TIMESTAMP: ");
            strMessage.append(strTime);
            strMessage.append("\r\n");
            strMessage.append("LOCATION : ");
            strMessage.append(className);
            strMessage.append("\r\n");
            strMessage.append("INFO     : ");
            strMessage.append(message);
            strMessage.append("\r\n----------------------------------------------------------------\r\n\r\n");
            fputs(strMessage.c_str(), logFile);

            #ifdef DEBUG
                fprintf(stderr, "%s", strMessage.c_str());
            #endif // DEBUG

            // Close file
            fclose(logFile);
        };
    };
};
