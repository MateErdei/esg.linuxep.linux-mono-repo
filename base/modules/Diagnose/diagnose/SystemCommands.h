/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <string>

namespace diagnose
{
    class SystemCommands
    {
    public:
        explicit SystemCommands(const std::string& destination);

        /*
         * runs a command and writes the output to a file
         */
        int runCommand(const std::string& command, const std::string& filename);

        /*
         * gets the path of the executable
         */
        std::string getExecutablePath(const std::string executableName);

        /*
         * Archive the diagnose output into a tar.gz ready for sending to Sophos.
         */
        void tarDiagnoseFolder(const std::string& srcPath, const std::string& destPath);

    private:
        std::string m_destination;
        Common::FileSystem::FileSystemImpl m_fileSystem;
    };
} // namespace diagnose
