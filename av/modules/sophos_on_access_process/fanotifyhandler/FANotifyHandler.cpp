//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FANotifyHandler.h"
#include "Logger.h"

#include "common/SaferStrerror.h"

#include <fcntl.h>
#include <stdint.h>
#include <sys/fanotify.h>

using namespace sophos_on_access_process::fanotifyhandler;

FANotifyHandler::FANotifyHandler()
{
    m_fd.reset();

    int fanotify_fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT, O_RDONLY | O_CLOEXEC | O_LARGEFILE);
    if (fanotify_fd == -1)
    {
        LOGERROR("Unable to initialise fanotify: " << common::safer_strerror(errno) << ". On Access Scanning disabled");
        return;
    }
    LOGINFO("FANotify successfully initialised");

    m_fd.reset(fanotify_fd);
    LOGINFO("FANotify FD set to " << m_fd.fd());
}

int FANotifyHandler::faNotifyFd()
{
    if (!m_fd.valid())
    {
        LOGDEBUG("FANotify FD not valid " << m_fd.fd());
    }

    return m_fd.fd();
}

FANotifyHandler::~FANotifyHandler()
{
    m_fd.close();
}