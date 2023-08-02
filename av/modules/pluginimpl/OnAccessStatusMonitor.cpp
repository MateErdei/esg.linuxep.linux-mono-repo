// Copyright 2023 Sophos Limited. All rights reserved.

#include "OnAccessStatusMonitor.h"

#include "Logger.h"

#include "common/ApplicationPaths.h"
#include "common/InotifyFD.h"
#include "common/SaferStrerror.h"
#include "common/StatusFile.h"

#include <cassert>

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/poll.h>

using namespace Plugin;

namespace
{
    constexpr auto INOTIFY_MASK = IN_CLOSE_WRITE;

    class InotifyFD : public common::InotifyFD
    {
    public:
        explicit InotifyFD(const sophos_filesystem::path& directory);
    };

    InotifyFD::InotifyFD(const sophos_filesystem::path& path)
    {
        m_watchDescriptor = inotify_add_watch(m_inotifyFD.fd(), path.c_str(), INOTIFY_MASK);
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

    using InotifyFD_t = InotifyFD;
}

OnAccessStatusMonitor::OnAccessStatusMonitor(Plugin::PluginCallbackSharedPtr callback)
    : m_callback(std::move(callback))
{
}

void OnAccessStatusMonitor::run()
{
    LOGDEBUG("Starting OnAccessStatusMonitor");

    // Setup inotify to wait for status file to change
    auto path = Plugin::getOnAccessStatusPath();
    statusFileChanged();


    auto watcher = std::make_unique<InotifyFD_t>(path);

    struct pollfd fds[] {
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = -1, .events = POLLIN, .revents = 0 }, // Watcher
    };

    // We need timeouts, since sometimes we end up reading old values from the status file after inotify
    const struct timespec LONG_TIMEOUT
    {
        .tv_sec = 600,
        .tv_nsec = 0
    };
    const struct timespec SHORT_TIMEOUT
    {
        .tv_sec = 10,
        .tv_nsec = 0,
    };

    announceThreadStarted();
    const struct timespec* timeout = &LONG_TIMEOUT;

    while (true)
    {
        if (watcher && watcher->valid())
        {
            fds[1].fd = watcher->getFD();
        }
        else
        {
            // No Inotify watch, so we need to poll frequently
            fds[1].fd = -1;
            timeout = &SHORT_TIMEOUT;
        }

        assert(timeout != nullptr);
//        LOGDEBUG("ppoll timeout " << timeout->tv_sec);
        auto ret = ::ppoll(fds, std::size(fds), timeout, nullptr);

        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
            break;
        }
        else if (ret == 0)
        {
            LOGDEBUG("Timeout from ppoll watching on-access status");
            watcher.reset();
            timeout = &LONG_TIMEOUT; // wait longer time if we get timeout
        }
        else if ((fds[0].revents & POLLERR) != 0)
        {
            LOGERROR("Shutting down OnAccessStatusMonitor, error from shutdown notify pipe");
            break;
        }
        else if ((fds[0].revents & POLLIN) != 0)
        {
            if (stopRequested())
            {
                LOGDEBUG("Got stop request, exiting OnAccessStatusMonitor");
                break;
            }
        }
        else if ((fds[1].revents & POLLERR) != 0)
        {
            LOGERROR("Reset OnAccessStatusMonitor, error from inotify");
            watcher.reset();
        }
        else if ((fds[1].revents & POLLIN) != 0)
        {
            constexpr size_t EVENT_SIZE = sizeof (struct inotify_event);
            constexpr size_t EVENT_BUF_LEN = 1024 * ( EVENT_SIZE + 16 );

            char buffer[EVENT_BUF_LEN] __attribute__ ((aligned(__alignof__(struct inotify_event))));

            ssize_t length = ::read(watcher->getFD(), buffer, EVENT_BUF_LEN);

            if (length < 0)
            {
                LOGERROR("Failed to read event from inotify: " << common::safer_strerror(errno));
                watcher.reset();
            }
            else
            {
                // Evaluate if interesting file has changed - if required in future
                statusFileChanged();
            }

            timeout = &SHORT_TIMEOUT; // Wait short timeout after event - to allow disks to sync
        }

        if (!watcher)
        {
            watcher = std::make_unique<InotifyFD_t>(path);
            statusFileChanged();
        }
    }
}

void OnAccessStatusMonitor::statusFileChanged()
{
    auto path = Plugin::getOnAccessStatusPath();
    auto oldStatus = m_onAccessStatus;
    std::this_thread::sleep_for(std::chrono::milliseconds{1}); // wait for file to be written
    auto isEnabled = common::StatusFile::isEnabled(path);
    if (oldStatus != isEnabled)
    {
        m_callback->setOnAccessEnabled(isEnabled);
        if (m_callback->sendStatus())
        {
            LOGDEBUG("Updating SAV status as On-Access status is " << isEnabled);
        }
        m_onAccessStatus = isEnabled;
    }
}
