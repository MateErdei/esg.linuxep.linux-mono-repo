//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandler.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

#include <sstream>

#include <fcntl.h>

using namespace sophos_on_access_process::fanotifyhandler;

FanotifyHandler::FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper)
    : m_systemCallWrapper(std::move(systemCallWrapper))
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
    LOGINFO("Fanotify FD set to " << m_fd.fd());
}

int FanotifyHandler::getFd() const
{
    if (!m_fd.valid())
    {
        LOGDEBUG("Fanotify FD not valid " << m_fd.fd());
    }

    return m_fd.fd();
}

int FanotifyHandler::markMount(const std::string& path) const
{
    assert(m_systemCallWrapper);
    constexpr unsigned int flags = FAN_MARK_ADD | FAN_MARK_MOUNT;
    constexpr uint64_t mask = FAN_CLOSE_WRITE | FAN_OPEN;
    constexpr int dfd = FAN_NOFD;
    int result = m_systemCallWrapper->fanotify_mark(getFd(), flags, mask, dfd, path.c_str());
    if (result < 0)
    {
     processFaMarkError("markMount", path);
    }
    return result;
}

int FanotifyHandler::cacheFd(const int& fd, const std::string& path) const
{
    assert(m_systemCallWrapper);
    const unsigned int flags = FAN_MARK_ADD | FAN_MARK_IGNORED_MASK;
    const uint64_t mask = FAN_OPEN;
    int result = m_systemCallWrapper->fanotify_mark(getFd(), flags, mask, fd, nullptr);
    if (result < 0)
    {
        processFaMarkError("cacheFd", path);
    }
    return result;
}

void FanotifyHandler::processFaMarkError(const std::string& function, const std::string& path)
{
    LOGERROR("fanotify_mark failed: " << function << " : " << common::safer_strerror(errno) << " Path: " << path);
}

FanotifyHandler::~FanotifyHandler()
{
    m_fd.close();
}