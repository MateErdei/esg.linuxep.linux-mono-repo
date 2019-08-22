/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PidLockFile.h"

#include <Common/UtilityImpl/StrError.h>
#include <sys/file.h>

#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>

namespace Common
{
    namespace FileSystemImpl
    {
        using namespace Common::FileSystem;
        using namespace Common::UtilityImpl;
        PidLockFile::PidLockFile(const std::string& pidfile) : m_fileDescriptor(-1), m_pidfile(pidfile)
        {
            int localfd = pidLockUtils()->open(m_pidfile, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

            if (localfd == -1)
            {
                std::stringstream errorStream;
                errorStream << "Failed to open lock file " << m_pidfile << " because " << StrError(errno) << "("
                            << errno << ")";
                throw std::system_error(errno, std::generic_category(), errorStream.str());
            }

            if (pidLockUtils()->flock(localfd) == -1)
            {
                // unable to lock the file
                pidLockUtils()->close(localfd);
                std::stringstream errorStream;
                errorStream << "Failed to lock file " << m_pidfile << " because " << StrError(errno) << "(" << errno
                            << ")";
                throw std::system_error(errno, std::generic_category(), errorStream.str());
            }

            // truncate and write new PID
            if (pidLockUtils()->ftruncate(localfd, 0) != 0)
            {
                pidLockUtils()->close(localfd);
                std::stringstream errorStream;
                errorStream << "Failed to truncate pidfile " << m_pidfile << " because " << StrError(errno) << "("
                            << errno << ")";
                throw std::system_error(errno, std::generic_category(), errorStream.str());
            }

            // write new PID
            char pid[15];
            snprintf(pid, 14, "%ld", static_cast<long>(pidLockUtils()->getpid()));
            pid[14] = 0;
            size_t pidLen = strlen(pid);

            ssize_t written = pidLockUtils()->write(localfd, pid, pidLen);
            if (written < 0 || static_cast<size_t>(written) != pidLen)
            {
                pidLockUtils()->close(localfd);
                std::stringstream errorStream;
                errorStream << "Failed to write pid to pidfile" << m_pidfile << " because " << StrError(errno) << "("
                            << errno << ")";
                throw std::system_error(errno, std::generic_category(), errorStream.str());
            }

            m_fileDescriptor = localfd;
        }

        PidLockFile::~PidLockFile()
        {
            if (m_fileDescriptor >= 0)
            {
                pidLockUtils()->close(m_fileDescriptor);
                m_fileDescriptor = -1;
                pidLockUtils()->unlink(m_pidfile);
            }
        }

        int PidLockFileUtils::open(const std::string& pathname, int flags, mode_t mode) const
        {
            return ::open(pathname.c_str(), flags, mode);
        }

        int PidLockFileUtils::flock(int fd) const
        {
            return ::flock(fd, LOCK_EX | LOCK_NB);
        }

        int PidLockFileUtils::ftruncate(int fd, off_t length) const { return ::ftruncate(fd, length); }

        ssize_t PidLockFileUtils::write(int fd, const void* buf, size_t count) const { return ::write(fd, buf, count); }

        void PidLockFileUtils::close(int fd) const { ::close(fd); }

        void PidLockFileUtils::unlink(const std::string& pathname) const { ::unlink(pathname.c_str()); }

        __pid_t PidLockFileUtils::getpid() const { return ::getpid(); }
    } // namespace FileSystemImpl
} // namespace Common

std::unique_ptr<Common::FileSystem::ILockFileHolder> Common::FileSystem::acquireLockFile(const std::string& fullPath)
{
    std::unique_ptr<Common::FileSystem::ILockFileHolder> pidLock{ new Common::FileSystemImpl::PidLockFile(fullPath)};
    return pidLock;
}


Common::FileSystemImpl::IPidLockFileUtilsPtr& pidLockUtilsStaticPointer()
{
    static Common::FileSystemImpl::IPidLockFileUtilsPtr instance =
        Common::FileSystemImpl::IPidLockFileUtilsPtr(new Common::FileSystemImpl::PidLockFileUtils());
    return instance;
}

void Common::FileSystemImpl::replacePidLockUtils(Common::FileSystemImpl::IPidLockFileUtilsPtr pointerToReplace)
{
    pidLockUtilsStaticPointer().reset(pointerToReplace.release());
}

void Common::FileSystemImpl::restorePidLockUtils()
{
    pidLockUtilsStaticPointer().reset(new PidLockFileUtils());
}

Common::FileSystem::IPidLockFileUtils* Common::FileSystem::pidLockUtils()
{
    return pidLockUtilsStaticPointer().get();
}
