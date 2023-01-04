// Copyright 2022, Sophos Limited.  All rights reserved.

#include "InotifyFD.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

#include <cerrno>

#include <sys/inotify.h>

namespace plugin::manager::scanprocessmonitor
{
    static constexpr auto INOTIFY_MASK = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE;

    InotifyFD::InotifyFD(const fs::path& directory)
    {
        m_inotifyFD.reset(inotify_init());
        assert(m_inotifyFD.valid());
        m_watchDescriptor = inotify_add_watch(m_inotifyFD.fd(), directory.c_str(), INOTIFY_MASK);
        if (m_watchDescriptor < 0)
        {
            LOGERROR("Failed to watch directory: "
                     << directory << " - Unable to monitor DNS config files: " << common::safer_strerror(errno));
            m_inotifyFD.close();
        }
    }
}
