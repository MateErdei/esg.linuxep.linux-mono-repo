/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "InotifyFD.h"

#include "Logger.h"

#include <cerrno>
#include <cstring>
#include <sys/inotify.h>

using namespace plugin::manager::scanprocessmonitor;

InotifyFD::InotifyFD(fs::path directory)
    : m_inotifyFD(inotify_init())
    , m_watchDescriptor(inotify_add_watch(m_inotifyFD.fd(), directory.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO))
{
    if (m_watchDescriptor < 0)
    {
        LOGERROR("Failed to watch directory: Unable to monitor DNS config files" << strerror(errno));
        // inotifyFD automatically closed
    }
}

InotifyFD::~InotifyFD()
{
    inotify_rm_watch(m_inotifyFD.fd(), m_watchDescriptor);
    m_inotifyFD.close();
}

int InotifyFD::getFD()
{
    return m_inotifyFD.fd();
}
