// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IPidLockFile.h"

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"

#include <string>

namespace common
{
    /**
     * Create a lock file in the specified location, containing the current PID.
     * When destroyed, delete the PidLockFile.
     */
    class PidLockFile : public IPidLockFile
    {
    public:
        explicit PidLockFile(const std::string& pidfile);
        virtual ~PidLockFile();

        static bool isPidFileLocked(const std::string& pidfile, const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls);

    private:
        std::string m_pidfile;
        datatypes::AutoFd m_fd;
    };
}
