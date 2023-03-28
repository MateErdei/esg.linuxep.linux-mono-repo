/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/FileSystem/IFileSystem.h>

#include <string>

namespace diagnose
{
    constexpr int GL_10mbSize = 10 * 1024 * 1024;
    const int GL_ProcTimeoutMilliSecs = 500;
    const int GL_ProcMaxRetries = 20;

    class SystemCommands
    {
    public:
        explicit SystemCommands(const std::string& destination);

        /*
         * runs a command and writes the output to a file
         */
        int runCommand(const std::string& command
                       , std::vector<std::string> arguments
                       , const std::string& filename
                       , const std::vector<u_int16_t>& exitcodes= {}
                       , const int& retries = GL_ProcMaxRetries)
            const;

        /*
         * Archive the diagnose output into a tar.gz ready for sending to Sophos.
         */
        void tarDiagnoseFolder(const std::string& srcPath, const std::string& destPath) const;
        /*
        * Archive the diagnose output into a zip ready for upload to Central location
        */
        void zipDiagnoseFolder(const std::string& srcPath, const std::string& destPath) const;

    private:
        std::string runCommandOutputToString(const std::string& command, std::vector<std::string> args, const std::vector<u_int16_t>& exitcodes, const int& retries) const;

        std::string m_destination;
    };
} // namespace diagnose
