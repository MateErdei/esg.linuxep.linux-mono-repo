// Copyright 2022, Sophos Limited.  All rights reserved.

#include "PidLockFile.h"

#include "Logger.h"
#include "PidLockFileException.h"

#include "common/SaferStrerror.h"
#include "datatypes/AutoFd.h"
#include "datatypes/sophos_filesystem.h"

#include <sstream>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace common;
namespace fs = sophos_filesystem;


PidLockFile::PidLockFile(const std::string& pidfile)
    : m_pidfile(pidfile),m_fd(-1)
{
    // open for write
    int localfd = open(pidfile.c_str(), O_RDWR | O_CREAT
#ifdef O_CLOEXEC
                                            | O_CLOEXEC
#endif
                       , S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

    if (localfd == -1)
    {
        std::ostringstream ost;
        ost << "Unable to open lock file " << pidfile << " because " <<strerror(errno) << "(" << errno << ")";

        LOGERROR("ERROR:" << ost.str());
        throw PidLockFileException(ost.str());
    }
    fs::permissions(pidfile.c_str(), fs::perms::group_read | fs::perms::others_read,
                    fs::perm_options::add);

    // lock
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(localfd, F_SETLKW, &fl) == -1)
    {
        std::ostringstream ost;
        ost << "Unable to lock lock file "<< pidfile << " because " << strerror(errno) << "(" << errno << ")";

        // unable to lock the file
        LOGERROR("ERROR:" << ost.str());
        close(localfd);
        throw PidLockFileException(ost.str());
    }

    // truncate and write new PID
    if (ftruncate(localfd,0) != 0)
    {
        LOGDEBUG("Failed to truncate pidfile: " << errno);
        close(localfd);
        throw PidLockFileException("Unabled to truncate lock file");
    }

    // write new PID
    char pid[15];
    snprintf(pid,14,"%ld",static_cast<long>(getpid()));
    pid[14] = 0;
    size_t pidLen = strlen(pid);

    ssize_t written = write(localfd,pid,pidLen);
    if (written < 0 || static_cast<size_t>(written) != pidLen)
    {
        LOGDEBUG("Failed to write pid to pidfile: " << errno);
        close(localfd);
        throw PidLockFileException("Unabled to pid to pid lock file");
    }

    m_fd = localfd;
    LOGINFO("Lock taken on: " << pidfile);
}

PidLockFile::~PidLockFile()
{
    LOGINFO("Closing lock file");
    if (m_fd >= 0)
    {
        close(m_fd);
        m_fd = -1;
        unlink(m_pidfile.c_str());
    }
}

bool PidLockFile::isPidFileLocked(const std::string& pidfile, const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls)
{
    datatypes::AutoFd fd(sysCalls->_open(pidfile.c_str(), O_RDONLY, 0644));
    if (!fd.valid())
    {
        auto buf = common::safer_strerror(errno);
        LOGDEBUG("Unable to open PID file " << pidfile << " (" << buf << "), assume process not running");
        return false;
    }

    struct flock fl;
    fl.l_type    = F_RDLCK;   /* Test for any lock on any part of file. */
    fl.l_whence  = SEEK_SET;
    fl.l_start   = 0;
    fl.l_len     = 50;
    sysCalls->fcntl(fd.get(), F_GETLK, &fl);  /* Overwrites lock structure with preventors. */
    if (fl.l_type == F_WRLCK)
    {
        LOGDEBUG("Unable to acquire lock on " << pidfile << " as process " << fl.l_pid << " already has a write lock");
        return true;
    }
    else if (fl.l_type == F_RDLCK)
    {
        LOGDEBUG("Unable to acquire lock on " << pidfile << " as process " << fl.l_pid << " already has a read lock");
        return true;
    }
    else
    {
        LOGDEBUG("Lock acquired on PID file " << pidfile << ", assume process not running");
    }
    return false;
}
