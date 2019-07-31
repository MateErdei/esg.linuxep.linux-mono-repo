/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/FileSystem/IFileSystem.h>

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
        int runCommand(const std::string& command, std::vector<std::string> arguments, const std::string& filename)
            const;

        /*
         * Archive the diagnose output into a tar.gz ready for sending to Sophos.
         */
        void tarDiagnoseFolder(const std::string& srcPath, const std::string& destPath) const;

    private:
        std::string getExecutablePath(const std::string& executableName) const;
        std::string runCommandOutputToString(const std::string& command, std::vector<std::string> args) const;

        std::string m_destination;
    };
} // namespace diagnose
