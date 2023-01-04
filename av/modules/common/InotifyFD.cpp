// Copyright 2023 Sophos Limited. All rights reserved.

#include "InotifyFD.h"

#include "Logger.h"
#include "SaferStrerror.h"

#include <sys/inotify.h>

using namespace common;

InotifyFD::InotifyFD(const sophos_filesystem::path& path) :
    m_inotifyFD(inotify_init()),
    m_watchDescriptor(inotify_add_watch(m_inotifyFD.fd(), path.c_str(), IN_CLOSE_WRITE))
{
    if (m_watchDescriptor < 0)
    {
        if (errno == ENOENT)
        {
            // File doesn't exist yet
            LOGDEBUG("Failed to watch status file " << path << " as it doesn't exist: falling back to polling");
        }
        else
        {
            LOGERROR("Failed to watch path: " << path << ": " << common::safer_strerror(errno));
        }
        m_inotifyFD.close();
    }
}

InotifyFD::~InotifyFD()
{
    if (m_watchDescriptor >= 0)
    {
        inotify_rm_watch(m_inotifyFD.fd(), m_watchDescriptor);
    }
    m_inotifyFD.close();
}

int InotifyFD::getFD()
{
    return m_inotifyFD.fd();
}

