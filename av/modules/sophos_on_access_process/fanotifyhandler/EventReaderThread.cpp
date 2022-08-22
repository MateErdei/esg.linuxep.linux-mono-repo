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

static const size_t FAN_BUFFER_SIZE = FAN_EVENT_METADATA_LEN * 2 - 1;

EventReaderThread::EventReaderThread(int fanotifyFD, datatypes::ISystemCallWrapperSharedPtr sysCalls)
    : m_fanotifyfd(fanotifyFD)
    , m_sysCalls(sysCalls)
{
    m_pid = getpid();
}

bool EventReaderThread::handleFanotifyEvent()
{
    char buf[FAN_BUFFER_SIZE];

    errno = 0;
    ssize_t len = m_sysCalls->read(m_fanotifyfd, buf, sizeof(buf));

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

        if (metadata->pid == m_pid)
        {
            LOGDEBUG("Skip event caused by soapd");
            continue;
        }

        auto path = getFilePathFromFd(eventFd.get());
        auto uid = getUidFromPid(metadata->pid);
        // TODO: Handle process exclusions

        if (metadata->mask & FAN_OPEN)
        {
            LOGINFO("On-open event for " << path << " from PID " << metadata->pid << " and UID " << uid);
        }
        else if (metadata->mask & FAN_CLOSE_WRITE)
        {
            LOGINFO("On-close event for " << path << " from PID " << metadata->pid << " and UID " << uid);
        }
        else
        {
            LOGDEBUG("unknown operation mask: " << std::hex << metadata->mask);
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

    return 0;
}

void EventReaderThread::run()
{
    int exitFD = m_notifyPipe.readFd();
    const int num_fds = 2;
    struct pollfd fds[num_fds];

    fds[0].fd = exitFD;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = m_fanotifyfd;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    announceThreadStarted();

    while (true)
    {
        int ret = m_sysCalls->ppoll(fds, num_fds, nullptr, nullptr);

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
            LOGDEBUG("Stopping the reading of FANotify events");
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            if (!handleFanotifyEvent())
            {
                LOGDEBUG("Stopping the reading of FANotify events");
                break;
            }
        }
    }
}
