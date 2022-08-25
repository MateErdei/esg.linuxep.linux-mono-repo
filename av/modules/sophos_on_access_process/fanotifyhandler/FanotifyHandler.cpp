//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandler.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

#include <sstream>

#include <fcntl.h>
#include <sys/fanotify.h>

using namespace sophos_on_access_process::fanotifyhandler;

FanotifyHandler::FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper)
{
    int fanotifyFd = systemCallWrapper->fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT, O_RDONLY | O_CLOEXEC | O_LARGEFILE);
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

FanotifyHandler::~FanotifyHandler()
{
    m_fd.close();
}