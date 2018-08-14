#include "Files.h"

/*********************************************************************
* Read file
*********************************************************************/
char* Files::ReadFile (std::string fileName)
{
    std::ifstream file(fileName, std::ifstream::in);
    if (!file.is_open())
    {
        std::string message = "Can't open file: ";
        message.append(fileName);
        Error::WriteLog("ERROR", "Files::ReadFile", message.c_str());
        return (NULL);
    }

    // get length of file:
    file.seekg (0, file.end);
    int fileSize = file.tellg();
    file.seekg (0, file.beg);

    char * memblock = new char [fileSize];
    file.seekg (0, std::ios::beg);
    file.read (memblock, fileSize);
    file.close();

    return (memblock);
};

/*********************************************************************
* Copy file
*********************************************************************/
void Files::CopyFile (std::string from, std::string to)
{
    std::ifstream  src(from, std::ios::binary);
    std::ofstream  dst(to,   std::ios::binary);

    dst << src.rdbuf();
};
