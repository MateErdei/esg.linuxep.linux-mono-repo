/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigMonitor.h"

#include "Logger.h"

#include "datatypes/AutoFd.h"

#include <modules/common/FDUtils.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/inotify.h>
#include <sys/types.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

using namespace plugin::manager::scanprocessmonitor;

ConfigMonitor::ConfigMonitor(Common::Threads::NotifyPipe& pipe,
                             std::string base)
    : m_configChangedPipe(pipe), m_base(std::move(base))
{
}

static const std::vector<std::string>& interestingFiles()
{
    static const std::vector<std::string> INTERESTING_FILES
        {
            "nsswitch.conf",
            "resolv.conf",
            "ld.so.cache",
            "host.conf",
            "hosts",
        };
    return INTERESTING_FILES;
}

static inline bool isInteresting(const std::string& basename)
{
//    LOGDEBUG("isInteresting:"<<basename);
    const auto& INTERESTING_FILES = interestingFiles();
    return (std::find(INTERESTING_FILES.begin(), INTERESTING_FILES.end(), basename) != INTERESTING_FILES.end());
}

void ConfigMonitor::run()
{
    datatypes::AutoFd inotifyFD(
        inotify_init()
    );
    if (inotifyFD.fd() < 0)
    {
        LOGERROR("Failed to initialise inotify: Unable to monitor DNS config files: " << strerror(errno));
        return;
    }

    // Add a watch for changes to the directory base ("/etc")
    int wd = inotify_add_watch(inotifyFD.fd(), m_base.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO);
    if (wd < 0)
    {
        LOGERROR("Failed to watch directory: Unable to monitor DNS config files" << strerror(errno));
        return; // inotifyFD automatically closed
    }

    // Only announce we've started once the watch is setup.
    announceThreadStarted();

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;
    max_fd = FDUtils::addFD(&readfds, m_notifyPipe.readFd(), max_fd);
    max_fd = FDUtils::addFD(&readfds, inotifyFD.fd(), max_fd);

    while(true)
    {
        fd_set temp_readfds = readfds;
        int active = ::pselect(max_fd+1, &temp_readfds, nullptr, nullptr, nullptr, nullptr);

        if (active < 0 and errno != EINTR)
        {
            LOGERROR("failure in ConfigMonitor: pselect failed: " << strerror(errno));
            break;
        }

        if (stopRequested())
        {
            break;
        }

        if (FDUtils::fd_isset(inotifyFD.fd(), &temp_readfds))
        {
//            LOGDEBUG("inotified");
            // Something changed under m_base (/etc)
            char buffer[EVENT_BUF_LEN];
            ssize_t length = ::read( inotifyFD.fd(), buffer, EVENT_BUF_LEN );

            if (length < 0)
            {
                LOGERROR("Failed to read event from inotify: "<< strerror(errno));
                break;
            }

            uint32_t i = 0;
            while (i < length)
            {
                auto* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);
                if (event->len)
                {
                    // Don't care if it's close-after-write or rename/move
                    if (isInteresting(event->name))
                    {
                        LOGINFO("System configuration updated");
                        m_configChangedPipe.notify();
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        }
    }

    inotify_rm_watch(inotifyFD.fd(), wd);
    inotifyFD.close();
}
