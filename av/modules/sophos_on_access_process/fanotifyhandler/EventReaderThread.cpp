//Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "EventReaderThread.h"
#include "Logger.h"
#include "fanotify.h"

#include "common/SaferStrerror.h"

// Standard C++
#include <memory>
// Standard C
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>


using namespace sophos_on_access_process::fanotifyhandler;

static const size_t BUFFER_SIZE = FAN_EVENT_METADATA_LEN * 2 - 1;

EventReaderThread::EventReaderThread(int fanotifyFD)
    : m_fanotifyfd(fanotifyFD)
{
    m_pid = getpid();
    m_ppid = getppid();
    m_sid = getsid(m_pid);
}

void EventReaderThread::handleFanotifyEvent()
{
    char buf[BUFFER_SIZE];
    pid_t mypid = m_pid;
    pid_t myppid = m_ppid;
    pid_t mysid = m_sid;

    errno = 0;
    ssize_t len = ::read(m_fanotifyfd, buf, sizeof(buf));
    int error = errno;

    // Verify we got something.
    if (len <= 0)
    {
        if (error == EAGAIN)
        {
            // Another thread got it:
            return;
        }

        // nothing actually there - maybe another thread got it
        LOGDEBUG(
            "no event or error: " << len <<
            " (" << error << " "<< common::safer_strerror(error)<<")"
        );
        return;
    }
    else
    {
        LOGDEBUG("got event: size " << len);
    }

    struct fanotify_event_metadata* metadata = reinterpret_cast<struct fanotify_event_metadata*>(buf);

    for (; FAN_EVENT_OK(metadata, len); metadata = FAN_EVENT_NEXT(metadata, len))
    {
        if (metadata->vers < 2)
        {
            LOGERROR("fanotify kernel version too old" << metadata->vers);
            throw std::runtime_error("fanotify kernel version too old");
        }

        if (metadata->vers != FANOTIFY_METADATA_VERSION)
        {
            LOGERROR("fanotify wrong protocol version " << metadata->vers);
            throw std::runtime_error("fanotify wrong protocol version");
        }

        if (metadata->fd < 0)
        {
            LOGDEBUG("Got fanotify metadata event without fd");
            continue;
        }

        if (metadata->pid == mypid || metadata->pid == myppid || metadata->pid == 1)
        {
            LOGDEBUG("Skip event caused by soapd");
            continue;
        }

        // Check the session ID and whitelist any process that is in our session.
        if (getsid(metadata->pid) == mysid)
        {
            LOGDEBUG("Event received from process in our session. Allowing pid " << metadata->pid);
            continue;
        }

        // TODO: Handle process exclusions

        if (metadata->mask & FAN_OPEN_PERM)
        {
            LOGINFO("On-open event from PID " << metadata->pid << " for FD " << metadata->fd);
        }
        else if (metadata->mask & FAN_CLOSE)
        {
            LOGINFO("On-close event from PID " << metadata->pid << " for FD " << metadata->fd);
        }
        else
        {
            LOGDEBUG("unknown operation mask: " << std::hex << metadata->mask);
        }
    }
}

void EventReaderThread::run()
{
    struct timespec pollTimeout = {5,0};

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
        int ret = ::ppoll(fds, num_fds, &pollTimeout, nullptr);

        if (ret < 0)
        {
            if (errno != EINTR)
            {
                // Error
                LOGDEBUG("Error from poll: " << errno);
            }
        }
        else if (ret == 0)
        {
            // TIMEOUT
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            LOGDEBUG("Stopping the reading of FANotify events");
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            handleFanotifyEvent();
        }
    }
    LOGDEBUG("Exiting EventReaderThread::run()");
}
