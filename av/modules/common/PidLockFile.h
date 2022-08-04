// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <string>

namespace common
{
    /**
     * Create a lock file in the specified location, containing the current PID.
     * When destroyed, delete the PidLockFile.
     */
    class PidLockFile
    {
    public:
        explicit PidLockFile(const std::string& pidfile);
        virtual ~PidLockFile();

    private:
        std::string m_pidfile;
        int m_fd;
    };
}
