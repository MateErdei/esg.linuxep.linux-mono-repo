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
        explicit SystemCommands(std::string destination);

        /*
         * runs a command and writes the output to a file
         */
        int runCommand(const std::string& command, const std::string& filename);

        /*
         * Archive the diagnose output into a tar.gz ready for sending to Sophos.
         */
        void tarDiagnoseFolder(const std::string& dirPath);

    private:
        std::string m_destination;
        bool isSafeToDelete(const std::string& path );
        void cleanupDir(const std::string& dirPath);
        void cleanupDirs(const std::string& dirPath);
    };
} // namespace diagnose
