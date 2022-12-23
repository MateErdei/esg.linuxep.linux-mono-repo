// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IPidLockFile.h"

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"

#include <string>

#include <sys/types.h>

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
        ~PidLockFile() override;

        static bool isPidFileLocked(const std::string& pidfile, const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls);

        /**
         * Get the PID from the lock file, if it is locked.
         * @param pidfile
         * @param sysCalls
         * @return 0 if the file isn't locked
         */
        static pid_t getPidIfLocked(const std::string& pidfile, const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls);

    private:
        std::string m_pidfile;
        datatypes::AutoFd m_fd;
    };
}
