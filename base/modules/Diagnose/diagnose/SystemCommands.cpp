/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "SystemCommands.h"
#include <stdlib.h>
#include <array>
#include <bits/unique_ptr.h>
#include "Common/FileSystemImpl/FileSystemImpl.h"

namespace diagnose
{
    SystemCommands::SystemCommands(std::string destination) :
    m_destination(destination)
    {

    }

    std::string SystemCommands::exec(const std::string& cmd)
    {
        std::array<char, 128> buffer{};
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }
        return result;
    }

    void SystemCommands::runCommandOutputToFile(std::string command, std::string filename)
    {
        std::string result = exec(command);
        m_fileSystem.writeFile(m_destination+filename, result);
    }
}
