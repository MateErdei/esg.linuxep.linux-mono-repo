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
     * If changePidGroup is set to true the group of the pid file will be set to sophos-spl-group, to be used for processes that run as root
     * If changePidGroup is set to false the groupId of the pid file will default to the process pid
     *
     */
    class PidLockFile : public IPidLockFile
    {
    public:
        explicit PidLockFile(const std::string& pidfile, bool changePidGroup=false);
        virtual ~PidLockFile();

        static bool isPidFileLocked(const std::string& pidfile, const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls);

    private:
        std::string m_pidfile;
        datatypes::AutoFd m_fd;
    };
}
