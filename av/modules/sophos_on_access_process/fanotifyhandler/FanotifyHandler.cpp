//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandler.h"

#include "Logger.h"

#include "common/ApplicationPaths.h"
#include "common/SaferStrerror.h"

#include <fstream>
#include <sstream>
#include <tuple>

#include <fcntl.h>

using namespace sophos_on_access_process::fanotifyhandler;

FanotifyHandler::FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper)
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
    assert(m_systemCallWrapper);
    int fanotifyFd = m_systemCallWrapper->fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS, O_RDONLY | O_CLOEXEC | O_LARGEFILE);
    if (fanotifyFd == -1)
    {
        std::ofstream onaccessUnhealthyFlagFile(Plugin::getOnAccessUnhealthyFlagPath());
        std::stringstream errMsg;
        errMsg << "Unable to initialise fanotify: " << common::safer_strerror(errno);
        throw std::runtime_error(errMsg.str());
    }
    LOGINFO("Fanotify successfully initialised");

    auto fanotify_autofd = m_fd.lock();
    fanotify_autofd->reset(fanotifyFd);
    LOGDEBUG("Fanotify FD set to " << fanotify_autofd->fd());
}

void FanotifyHandler::close()
{
    std::remove(Plugin::getOnAccessUnhealthyFlagPath().c_str());
    auto fanotify_autofd = m_fd.lock();
    fanotify_autofd->close();
}

int FanotifyHandler::getFd() const
{
    auto fanotify_autofd = m_fd.lock();

    if (!fanotify_autofd->valid())
    {
        LOGERROR("Fanotify FD not valid " << fanotify_autofd->fd());
    }

    return fanotify_autofd->fd();
}

int FanotifyHandler::markMount(const std::string& path) const
{
    assert(m_systemCallWrapper);

    auto fanotify_autofd = m_fd.lock(); // Hold the lock since we can be called while fanotify being disabled
    int fanotify_fd = fanotify_autofd->fd();
    if (fanotify_fd < 0)
    {
        LOGWARN("Skipping markMount for " << path << " as fanotify disabled");
        return 0;
    }

    constexpr unsigned int flags = FAN_MARK_ADD | FAN_MARK_MOUNT;
    constexpr uint64_t mask = FAN_CLOSE_WRITE | FAN_OPEN;
    constexpr int dfd = FAN_NOFD;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, dfd, path.c_str());
    return processFaMarkError(result, "markMount", path);
}

int FanotifyHandler::unmarkMount(const std::string& path) const
{
    assert(m_systemCallWrapper);

    auto fanotify_autofd = m_fd.lock(); // Hold the lock since we can be called while fanotify being disabled
    int fanotify_fd = fanotify_autofd->fd();
    if (fanotify_fd < 0)
    {
        LOGWARN("Skipping unmarkMount for " << path << " as fanotify disabled");
        return 0;
    }

    constexpr unsigned int flags = FAN_MARK_REMOVE | FAN_MARK_MOUNT;
    constexpr uint64_t mask = FAN_CLOSE_WRITE | FAN_OPEN;
    constexpr int dfd = FAN_NOFD;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, dfd, path.c_str());
    return processFaMarkError(result, "unmarkMount", path);
}

int FanotifyHandler::cacheFd(const int& fd, const std::string& path) const
{
    assert(m_systemCallWrapper);
    int fanotify_fd = getFd(); // CacheFd only called while fanotify enabled
    if (fanotify_fd < 0)
    {
        LOGERROR("Skipping cacheFd for " << path << " as fanotify disabled");
        return 0;
    }

    constexpr unsigned int flags = FAN_MARK_ADD | FAN_MARK_IGNORED_MASK;
    constexpr uint64_t mask = FAN_OPEN;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, fd, nullptr);
    return processFaMarkError(result, "cacheFd", path);
}

int FanotifyHandler::uncacheFd(const int& fd, const std::string& path) const
{
    assert(m_systemCallWrapper);
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
    auto fanotify_autofd = m_fd.lock();
    int fanotify_fd = fanotify_autofd->fd(); // Don't call getFd() since we need to hold the lock
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
    // TODO: Remove this condition once LINUXDAR-5803 is fixed
    if (function == "unmarkMount")
    {
        LOGWARN(logMsg.str());
    }
    else
    {
        LOGERROR(logMsg.str());
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
