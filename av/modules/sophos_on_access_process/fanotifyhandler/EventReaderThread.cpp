//Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "EventReaderThread.h"
#include "Logger.h"

#include "common/SaferStrerror.h"
#include "datatypes/AutoFd.h"

// Standard C++
#include <memory>
// Standard C
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <string.h>
#include <sys/fanotify.h>
#include <sys/stat.h>
#include <unistd.h>


using namespace sophos_on_access_process::fanotifyhandler;

// Set the buffer size to the size of one memory page
static constexpr size_t FAN_BUFFER_SIZE = 4096;

EventReaderThread::EventReaderThread(
    int fanotifyFD,
    datatypes::ISystemCallWrapperSharedPtr sysCalls,
    const fs::path& pluginInstall,
    std::shared_ptr<sophos_on_access_process::onaccessimpl::ScanRequestQueue> scanRequestQueue)
    : m_fanotifyFd(fanotifyFD)
    , m_sysCalls(sysCalls)
    , m_pluginLogDir(pluginInstall / "log")
    , m_scanRequestQueue(scanRequestQueue)
    , m_pid(getpid())
{
}

bool EventReaderThread::handleFanotifyEvent()
{
    char buf[FAN_BUFFER_SIZE];

    errno = 0;
    ssize_t len = m_sysCalls->read(m_fanotifyFd, buf, sizeof(buf));

    // Verify we got something.
    if (len <= 0)
    {
        int error = errno;
        if (error == EAGAIN)
        {
            // Another thread got it:
            return true;
        }

        // nothing actually there - maybe another thread got it
        LOGDEBUG(
            "no event or error: " << len <<
            " (" << error << " "<< common::safer_strerror(error)<<")"
        );
        return false;
    }
    else
    {
        LOGDEBUG("got event: size " << len);
    }

    struct fanotify_event_metadata* metadata = reinterpret_cast<struct fanotify_event_metadata*>(buf);

    for (; FAN_EVENT_OK(metadata, len); metadata = FAN_EVENT_NEXT(metadata, len))
    {
        if (metadata->vers != FANOTIFY_METADATA_VERSION)
        {
            LOGERROR("fanotify wrong protocol version " << metadata->vers);
            return false;
        }

        if (metadata->fd < 0)
        {
            LOGDEBUG("Got fanotify metadata event without fd");
            continue;
        }
        datatypes::AutoFd eventFd(metadata->fd);

        auto path = getFilePathFromFd(eventFd.get());
        // Exclude events caused by AV logging to prevent recursive events
        if (path.rfind(m_pluginLogDir, 0) == 0)
        {
            continue;
        }

        if (metadata->pid == m_pid)
        {
            LOGDEBUG("Skip event caused by soapd");
            continue;
        }

        auto uid = getUidFromPid(metadata->pid);
        // TODO: Handle process exclusions

        if (metadata->mask & FAN_OPEN)
        {
            LOGINFO("On-open event for " << path << " from PID " << metadata->pid << " and UID " << uid);
        }
        else if (metadata->mask & FAN_CLOSE_WRITE)
        {
            // TODO: optimize this by using emplace instead of push
            LOGINFO("On-close event for " << path << " from PID " << metadata->pid << " and UID " << uid);
            auto scanRequest = std::make_shared<scan_messages::ClientScanRequest>();
            scanRequest->setPath("");
            scanRequest->setScanType(E_SCAN_TYPE_ON_ACCESS);
            scanRequest->setUserID(std::to_string(uid));
            if (!m_scanRequestQueue->push(scanRequest, eventFd))
            {
                LOGERROR("Failed to add scan request to queue. Path will not be scanned: " << path);
            }
        }
        else
        {
            LOGDEBUG("unknown operation mask: " << std::hex << metadata->mask << std::dec);
        }
    }
    return true;
}

std::string EventReaderThread::getFilePathFromFd(int fd)
{
    char buffer[PATH_MAX];

    ssize_t len;

    if (fd <= 0)
        return "";

    std::stringstream procFdPath;
    procFdPath << "/proc/self/fd/" << fd;
    if ((len = m_sysCalls->readlink(procFdPath.str().c_str(), buffer, PATH_MAX - 1)) < 0)
    {
        return "";
    }

    buffer[len] = '\0';
    return buffer;
}

uid_t EventReaderThread::getUidFromPid(pid_t pid)
{
    std::stringstream procPidPath;
    procPidPath << "/proc/" << pid;
    struct ::stat statbuf{};

    int ret = m_sysCalls->_stat(procPidPath.str().c_str(), &statbuf);
    if (ret == 0)
    {
        return statbuf.st_uid;
    }

    // return invalid UID
    return static_cast<uid_t>(-1);
}

void EventReaderThread::run()
{
    struct pollfd fds[] {
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = m_fanotifyFd, .events = POLLIN, .revents = 0 },
    };

    announceThreadStarted();

    while (true)
    {
        int ret = m_sysCalls->ppoll(fds, std::size(fds), nullptr, nullptr);

        if (ret < 0)
        {
            if (errno != EINTR)
            {
                // Error
                LOGDEBUG("Error from poll: " << errno);
            }
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            LOGDEBUG("Stopping the reading of Fanotify events");
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            if (!handleFanotifyEvent())
            {
                LOGDEBUG("Stopping the reading of Fanotify events");
                break;
            }
        }
    }
}
