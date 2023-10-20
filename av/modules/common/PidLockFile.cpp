// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "PidLockFile.h"

#include "Logger.h"
#include "PidLockFileException.h"
#include "SaferStrerror.h"

#include "datatypes/AutoFd.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/FileSystem/IFilePermissions.h"

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sstream>

using namespace common;
using namespace Common::SystemCallWrapper;
namespace fs = sophos_filesystem;


PidLockFile::PidLockFile(const std::string& pidfile, bool changePidGroup)
    : m_pidfile(pidfile),m_fd(-1)
{
    // open for write
    datatypes::AutoFd local_fd(open(pidfile.c_str(), O_RDWR | O_CREAT | O_CLOEXEC
                       , S_IWUSR | S_IRUSR | S_IRGRP)
    );

    if (!local_fd.valid())
    {
        int error = errno;

        std::ostringstream ost;
        ost << "Unable to open lock file " << pidfile << " because " << common::safer_strerror(error) << "(" << error << ")";

        LOGERROR(ost.str());
        throw PidLockFileException(ost.str());
    }

    if(changePidGroup)
    {
        auto fp = Common::FileSystem::filePermissions();
        auto groupId = fp->getGroupId("sophos-spl-group");
        std::ignore = fchown(local_fd.get(), -1, groupId);
    }

    fchmod(local_fd.get(), 0640);

    // lock
    if (flock(local_fd.get(), LOCK_EX) == -1)
    {
        int error = errno;
        local_fd.close();

        std::ostringstream ost;
        ost << "Unable to lock lock file "<< pidfile << " because " << common::safer_strerror(error) << "(" << error << ")";

        // unable to lock the file
        LOGFATAL(ost.str());
        throw PidLockFileException(ost.str());
    }

    // truncate and write new PID
    if (ftruncate(local_fd.get(),0) != 0)
    {
        int error = errno;
        local_fd.close();

        std::ostringstream ost;
        ost << "Failed to truncate pidfile: " << common::safer_strerror(error) << "(" << error << ")";

        LOGFATAL(ost.str());
        throw PidLockFileException("Unable to truncate lock file");
    }

    // write new PID
    char pid[15];
    snprintf(pid,14,"%ld",static_cast<long>(getpid()));
    pid[14] = 0;
    size_t pidLen = strlen(pid);

    ssize_t written = write(local_fd.get(), pid, pidLen);
    if (written < 0 || static_cast<size_t>(written) != pidLen)
    {
        int error = errno;
        local_fd.close();
        LOGFATAL("Failed to write pid to pidfile: " << common::safer_strerror(error));
        throw PidLockFileException("Unable to write pid to pid lock file");
    }

    m_fd = std::move(local_fd);
    LOGINFO("Lock taken on: " << pidfile);
}

PidLockFile::~PidLockFile()
{
    LOGINFO("Closing lock file");
    if (m_fd.valid())
    {
        unlink(m_pidfile.c_str());
    }
    m_fd.reset();
}

namespace
{
    datatypes::AutoFd openPidLockFileIfLocked(const std::string& pidfile, const ISystemCallWrapperSharedPtr& sysCalls)
    {
        datatypes::AutoFd fd(sysCalls->_open(pidfile.c_str(), O_RDONLY, 0644));
        if (!fd.valid())
        {
            auto buf = common::safer_strerror(errno);
            LOGDEBUG("Unable to open PID file " << pidfile << " (" << buf << "), assume process not running");
            return datatypes::AutoFd{};
        }

        auto ret = sysCalls->flock(fd.get(), LOCK_EX | LOCK_NB);
        if (ret == 0)
        {
            LOGDEBUG("Lock acquired on PID file " << pidfile << ", assume process not running");
            return datatypes::AutoFd{};
        }

        int error = errno;
        if (error == EWOULDBLOCK)
        {
            LOGTRACE("Process is running as unable to acquire lock on " << pidfile << " as it is already locked");
            return fd;
        }
        else
        {
            LOGERROR("Failed to test lock on " << pidfile << " because " << common::safer_strerror(error) << "(" << error << ")");
            return datatypes::AutoFd{};
        }
    }
}

bool PidLockFile::isPidFileLocked(const std::string& pidfile, const ISystemCallWrapperSharedPtr& sysCalls)
{
    datatypes::AutoFd fd = openPidLockFileIfLocked(pidfile, sysCalls);
    return fd.valid();
}

pid_t PidLockFile::getPidIfLocked(
    const std::string& pidfile,
    const ISystemCallWrapperSharedPtr& sysCalls)
{
    datatypes::AutoFd fd = openPidLockFileIfLocked(pidfile, sysCalls);
    if (!fd.valid())
    {
        return 0;
    }
    char contents[32];
    auto read = sysCalls->read(fd.get(), contents, std::size(contents)-1);
    contents[read] = 0;
    return std::stoi(contents);
}
