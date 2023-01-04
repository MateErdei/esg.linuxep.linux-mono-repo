// Copyright 2023 Sophos Limited. All rights reserved.

#include "OnAccessStatusMonitor.h"

#include "Logger.h"

#include "common/ApplicationPaths.h"
#include "common/SaferStrerror.h"
#include "common/StatusFile.h"

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/poll.h>

using namespace Plugin;

namespace
{
    class InotifyFD
    {
    public:
        explicit InotifyFD(const sophos_filesystem::path& directory);

        ~InotifyFD();
        InotifyFD(const InotifyFD&) = delete;
        InotifyFD& operator=(const InotifyFD&) = delete;

        int getFD();

        bool valid()
        {
            return m_watchDescriptor >= 0;
        }

    private:
        datatypes::AutoFd m_inotifyFD;
        int m_watchDescriptor;
    };
}

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

    auto watcher = std::make_unique<InotifyFD>(path);

    struct pollfd fds[] {
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = -1, .events = POLLIN, .revents = 0 }, // Watcher
    };

    const struct timespec WITH_WATCHER_TIMEOUT
    {
        .tv_sec = 600,
        .tv_nsec = 0
    };
    const struct timespec WITHOUT_WATCHER_TIMEOUT
    {
        .tv_sec = 10,
        .tv_nsec = 0,
    };

    announceThreadStarted();

    while (true)
    {
        const struct timespec* timeout;
        if (watcher && watcher->valid())
        {
            fds[1].fd = watcher->getFD();
            timeout = &WITH_WATCHER_TIMEOUT;
        }
        else
        {
            fds[1].fd = -1;
            timeout = &WITHOUT_WATCHER_TIMEOUT;
        }

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
            LOGERROR("Shutting down OnAccessStatusMonitor, error from inotify");
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
                LOGDEBUG("Read " << length << " of inotify events");
                statusFileChanged();
            }
        }

        if (!watcher)
        {
            watcher = std::make_unique<InotifyFD>(path);
            statusFileChanged();
        }
    }
}

void OnAccessStatusMonitor::statusFileChanged()
{
    auto path = Plugin::getOnAccessStatusPath();
    auto isEnabled = common::StatusFile::isEnabled(path);
    m_callback->setOnAccessEnabled(isEnabled);
    m_callback->sendStatus();
}
