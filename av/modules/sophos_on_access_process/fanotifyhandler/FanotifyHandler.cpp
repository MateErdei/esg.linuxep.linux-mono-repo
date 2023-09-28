// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "FanotifyHandler.h"

#include "Logger.h"

#include "common/ApplicationPaths.h"
#include "common/SaferStrerror.h"

#include "Common/FileSystem/IFileSystem.h"

#include <sstream>
#include <tuple>

#include <fcntl.h>
#include <sys/fanotify.h>

using namespace sophos_on_access_process::fanotifyhandler;

FanotifyHandler::FanotifyHandler(Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  systemCallWrapper)
    : m_systemCallWrapper(std::move(systemCallWrapper))
{
    assert(m_systemCallWrapper);
}

FanotifyHandler::~FanotifyHandler()
{
    close();
}

void FanotifyHandler::init()
{
    auto fs = Common::FileSystem::fileSystem();
    constexpr unsigned int flags = FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS | FAN_NONBLOCK;
    constexpr unsigned int event_f_flags = O_RDONLY | O_CLOEXEC | O_LARGEFILE;
    int fanotifyFd = m_systemCallWrapper->fanotify_init(flags, event_f_flags);
    if (fanotifyFd == -1)
    {
        fs->writeFile(Plugin::getOnAccessUnhealthyFlagPath(), "");
        std::stringstream errMsg;
        errMsg << "Unable to initialise fanotify: " << common::safer_strerror(errno);
        throw std::runtime_error(errMsg.str());
    }

    fs->removeFile(Plugin::getOnAccessUnhealthyFlagPath(), true);

    auto fanotify_autoFd = m_fd.lock();
    fanotify_autoFd->reset(fanotifyFd);
    LOGDEBUG("Fanotify successfully initialised: Fanotify FD=" << fanotify_autoFd->fd());
}

void FanotifyHandler::close()
{
    auto fs = Common::FileSystem::fileSystem();
    fs->removeFile(Plugin::getOnAccessUnhealthyFlagPath(), true);

    auto fanotify_autoFd = m_fd.lock();
    fanotify_autoFd->close();
}

int FanotifyHandler::getFd() const
{
    auto fanotify_autoFd = m_fd.lock();

    if (!fanotify_autoFd->valid())
    {
        LOGERROR("Fanotify FD not valid " << fanotify_autoFd->fd());
    }

    return fanotify_autoFd->fd();
}

int FanotifyHandler::markMount(const std::string& path, bool onOpen, bool onClose)
{
    auto fanotify_autoFd = m_fd.lock(); // Hold the lock since we can be called while fanotify being disabled
    int fanotify_fd = fanotify_autoFd->fd();
    if (fanotify_fd < 0)
    {
        LOGDEBUG("Skipping markMount for " << path << " as fanotify disabled");
        return 0;
    }

    constexpr unsigned int flags = FAN_MARK_ADD | FAN_MARK_MOUNT;
    uint64_t mask = 0;
    if (onClose)
    {
        mask |= FAN_CLOSE_WRITE;
    }
    if (onOpen)
    {
        mask |= FAN_OPEN;
    }

    auto pos = markMap_.find(path);
    if (pos != markMap_.end())
    {
        auto oldMask = pos->second;
        if (oldMask == mask)
        {
            LOGDEBUG("Skipping markMount for " << path << " as already marked");
        }
        else
        {
            std::ignore = unmarkMount(path, fanotify_fd);
        }
    }

    constexpr int dfd = FAN_NOFD;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, dfd, path.c_str());
    markMap_[path] = mask;
    return processFaMarkError(result, "markMount", path);
}

int FanotifyHandler::unmarkMount(const std::string& path)
{
    auto fanotify_autoFd = m_fd.lock(); // Hold the lock since we can be called while fanotify being disabled
    int fanotify_fd = fanotify_autoFd->fd();

    if (fanotify_fd < 0)
    {
        LOGDEBUG("Skipping unmarkMount for " << path << " as fanotify disabled");
        return 0;
    }

    return unmarkMount(path, fanotify_fd);
}

int FanotifyHandler::unmarkMount(const std::string& path, int fanotify_fd)
{
    constexpr unsigned int flags = FAN_MARK_REMOVE | FAN_MARK_MOUNT;
    constexpr uint64_t mask = FAN_CLOSE_WRITE | FAN_OPEN;
    constexpr int dfd = FAN_NOFD;
    errno = 0;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, dfd, path.c_str());
    if (result < 0 && errno == ENOENT)
    {
        return 0; // ignore enoent, since it just means we haven't previously marked the mount point
    }
    markMap_.erase(path);
    return processFaMarkError(result, "unmarkMount", path);
}

int FanotifyHandler::cacheFd(const int& fd, const std::string& path, bool surviveModify) const
{
    int fanotify_fd = getFd(); // CacheFd only called while fanotify enabled
    if (fanotify_fd < 0)
    {
        LOGERROR("Skipping cacheFd for " << path << " as fanotify disabled");
        return 0;
    }

    unsigned int flags = FAN_MARK_ADD | FAN_MARK_IGNORED_MASK;
    if (surviveModify)
    {
        flags |= FAN_MARK_IGNORED_SURV_MODIFY;
    }
    constexpr uint64_t mask = FAN_OPEN;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, fd, nullptr);
    return processFaMarkError(result, "cacheFd", path);
}

int FanotifyHandler::uncacheFd(const int& fd, const std::string& path) const
{
    int fanotify_fd = getFd(); // uncacheFd only called while fanotify enabled
    if (fanotify_fd < 0)
    {
        LOGERROR("Skipping uncacheFd for " << path << " as fanotify disabled");
        return 0;
    }

    constexpr unsigned int flags = FAN_MARK_REMOVE | FAN_MARK_IGNORED_MASK;
    constexpr uint64_t mask = FAN_OPEN;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, fd, nullptr);
    return result;
}

int FanotifyHandler::clearCachedFiles() const
{
    auto fanotify_autoFd = m_fd.lock();
    int fanotify_fd = fanotify_autoFd->fd(); // Don't call getFd() since we need to hold the lock
    if (fanotify_fd < 0)
    {
        LOGINFO("Clearing cache skipped as fanotify disabled");
        return 0;
    }

    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, FAN_MARK_FLUSH, 0, FAN_NOFD, nullptr);
    return processFaMarkError(result, "clearCachedFiles", "fanotify fd");
}

void FanotifyHandler::updateComplete()
{
    LOGINFO("Clearing on-access cache");
    std::ignore = clearCachedFiles();
    LOGDEBUG("Cleared on-access cache");
}

void FanotifyHandler::processFaMarkError(const std::string& function, const std::string& path)
{
    std::stringstream logMsg;
    int error = errno;
    logMsg << "fanotify_mark failed in " << function << ": " << common::safer_strerror(error) << " for: " << path;

    switch (error)
    {
        case ENOENT:
            LOGDEBUG(logMsg.str());
            break;
        case EACCES:
            LOGWARN(logMsg.str());
            break;
        default:
            LOGERROR(logMsg.str());
            break;
    }
}

int FanotifyHandler::processFaMarkError(int result, const std::string& function, const std::string& path)
{
    if (result < 0)
    {
        processFaMarkError(function, path);
    }
    return result;
}
