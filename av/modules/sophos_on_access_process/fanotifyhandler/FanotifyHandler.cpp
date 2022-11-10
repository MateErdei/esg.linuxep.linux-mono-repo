//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandler.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

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
    int fanotifyFd = m_systemCallWrapper->fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT, O_RDONLY | O_CLOEXEC | O_LARGEFILE);
    if (fanotifyFd == -1)
    {
        std::stringstream errMsg;
        errMsg << "Unable to initialise fanotify: " << common::safer_strerror(errno);
        throw std::runtime_error(errMsg.str());
    }
    LOGINFO("Fanotify successfully initialised");

    m_fd.reset(fanotifyFd);
    LOGDEBUG("Fanotify FD set to " << m_fd.fd());
}

bool FanotifyHandler::isInitialised() const
{
    return m_fd.valid();
}

void FanotifyHandler::close()
{
    m_fd.close();
}

int FanotifyHandler::getFd() const
{
    if (!m_fd.valid())
    {
        LOGERROR("Fanotify FD not valid " << m_fd.fd());
    }

    return m_fd.fd();
}

int FanotifyHandler::markMount(const std::string& path) const
{
    assert(m_systemCallWrapper);

    int fanotify_fd = getFd();
    if (fanotify_fd < 0)
    {
        LOGERROR("Skipping markMount for " << path << " as fanotify disabled");
        return 0;
    }

    constexpr unsigned int flags = FAN_MARK_ADD | FAN_MARK_MOUNT;
    constexpr uint64_t mask = FAN_CLOSE_WRITE | FAN_OPEN;
    constexpr int dfd = FAN_NOFD;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, dfd, path.c_str());
    if (result < 0)
    {
        processFaMarkError("markMount", path);
    }
    return result;
}

int FanotifyHandler::unmarkMount(const std::string& path) const
{
    assert(m_systemCallWrapper);

    int fanotify_fd = getFd();
    if (fanotify_fd < 0)
    {
        LOGERROR("Skipping unmarkMount for " << path << " as fanotify disabled");
        return 0;
    }

    constexpr unsigned int flags = FAN_MARK_REMOVE | FAN_MARK_MOUNT;
    constexpr uint64_t mask = FAN_CLOSE_WRITE | FAN_OPEN;
    constexpr int dfd = FAN_NOFD;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, dfd, path.c_str());
    if (result < 0)
    {
        processFaMarkError("unmarkMount", path);
    }
    return result;
}

int FanotifyHandler::cacheFd(const int& fd, const std::string& path) const
{
    assert(m_systemCallWrapper);
    int fanotify_fd = getFd();
    if (fanotify_fd < 0)
    {
        LOGERROR("Skipping cacheFd for " << path << " as fanotify disabled");
        return 0;
    }


    const unsigned int flags = FAN_MARK_ADD | FAN_MARK_IGNORED_MASK;
    const uint64_t mask = FAN_OPEN;
    int result = m_systemCallWrapper->fanotify_mark(fanotify_fd, flags, mask, fd, nullptr);
    if (result < 0)
    {
        processFaMarkError("cacheFd", path);
    }
    return result;
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
    int fd = m_fd.fd(); // Don't call getFd() since it will log an error
    if (fd < 0)
    {
        LOGINFO("Clearing cache skipped as fanotify disabled");
        return 0;
    }

    int result = m_systemCallWrapper->fanotify_mark(getFd(), FAN_MARK_FLUSH, 0, FAN_NOFD, nullptr);
    if (result < 0)
    {
        processFaMarkError("clearCachedFiles", "");
    }
    return result;
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
