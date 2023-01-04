// Copyright 2023 Sophos Limited. All rights reserved.

#include "InotifyFD.h"

#include "Logger.h"
#include "SaferStrerror.h"

#include <sys/inotify.h>

using namespace common;

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

