//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandler.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

#include <sstream>

#include <fcntl.h>
#include <sys/fanotify.h>

using namespace sophos_on_access_process::fanotifyhandler;

FanotifyHandler::FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper)
: m_systemCallWrapper(systemCallWrapper)
{
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

int FanotifyHandler::clearCachedFiles() const
{
    int result = m_systemCallWrapper->fanotify_mark(getFd(), FAN_MARK_FLUSH, 0, 0, "");
    if (result < 0)
    {
        processFaMarkError("cacheFd");
    }
    return result;
}

void FanotifyHandler::processFaMarkError(const std::string& function) const
{
    LOGERROR("fanotify_mark failed: " << function << " : " << common::safer_strerror(errno));
}

void FanotifyHandler::updateComplete()
{
    LOGINFO("Clearing on-access cache");
    clearCachedFiles();
}

FanotifyHandler::~FanotifyHandler()
{
    m_fd.close();
}