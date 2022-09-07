//Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "EventReaderThread.h"
#include "Logger.h"

#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "datatypes/AutoFd.h"
// Package
#include "ProcUtils.h"
// Product
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// Standard C++
#include <memory>
#include <utility>
// Standard C
#include <cerrno>
#include <climits>
#include <poll.h>
#include <cstring>
#include <sys/fanotify.h>
#include <sys/stat.h>
#include <unistd.h>


namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::fanotifyhandler;

// Set the buffer size to the size of one memory page
static constexpr size_t FAN_BUFFER_SIZE = 4096;

EventReaderThread::EventReaderThread(
    int fanotifyFD,
    datatypes::ISystemCallWrapperSharedPtr sysCalls,
    const fs::path& pluginInstall,
    onaccessimpl::ScanRequestQueueSharedPtr scanRequestQueue)
    : m_fanotifyFd(fanotifyFD)
    , m_sysCalls(std::move(sysCalls))
    , m_pluginLogDir(pluginInstall / "log")
    , m_scanRequestQueue(std::move(scanRequestQueue))
    , m_pid(getpid())
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path PLUGIN_INSTALL = appConfig.getData("PLUGIN_INSTALL");
    m_processExclusionStem = PLUGIN_INSTALL.string() + "/";
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

        if (error == EMFILE)
        {
            LOGERROR("No more File Descriptors available. Restarting On Access");
            exit(EXIT_FAILURE);
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

    auto* metadata = reinterpret_cast<struct fanotify_event_metadata*>(buf);

    for (; FAN_EVENT_OK(metadata, len); metadata = FAN_EVENT_NEXT(metadata, len))
    {
        if (metadata->vers != FANOTIFY_METADATA_VERSION)
        {
            LOGERROR("fanotify wrong protocol version " << metadata->vers);
            return false;
        }

        auto eventFd = metadata->fd;
        if (eventFd < 0)
        {
            LOGDEBUG("Got fanotify metadata event without fd");
            continue;
        }

        auto scanRequest = std::make_shared<scan_messages::ClientScanRequest>();
        scanRequest->setFd(eventFd); // DONATED

        auto path = getFilePathFromFd(eventFd);
        //Either path was too long or fd was invalid
        if(path.empty())
        {
            continue;
        }

        // Exclude events caused by AV logging to prevent recursive events
        if (path.rfind(m_pluginLogDir, 0) == 0)
        {
            continue;
        }

        if (metadata->pid == m_pid)
        {
            LOGDEBUG("Event pid matches On Access pid, skipping event");
            continue;
        }
        auto executablePath = get_executable_path_from_pid(metadata->pid);

        if (!m_processExclusionStem.empty() && startswith(executablePath, m_processExclusionStem))
        {
            LOGDEBUG("Excluding SPL-AV process");
            continue;
        }


        auto uid = getUidFromPid(metadata->pid);

        // TODO: Handle process exclusions
        auto escapedPath = common::escapePathForLogging(path);

        bool validEvent = true;
        std::string eventType = "";
        if (metadata->mask & FAN_OPEN)
        {
            eventType = "open";
        }
        else if (metadata->mask & FAN_CLOSE_WRITE)
        {
            eventType = "close";
        }
        else
        {
            LOGDEBUG("unknown operation mask: " << std::hex << metadata->mask << std::dec);
            validEvent = false;
        }

        if (validEvent)
        {
            LOGINFO("On-" << eventType << " event for " << escapedPath << " from PID " << metadata->pid << " and UID " << uid);
            scanRequest->setPath(path);
            scanRequest->setScanType(E_SCAN_TYPE_ON_ACCESS);
            scanRequest->setUserID(std::to_string(uid));

            if (!m_scanRequestQueue->emplace(std::move(scanRequest)))
            {
                LOGWARN("Failed to add scan request to queue. Path will not be scanned: " << path);
            }
        }
    }
    return true;
}

std::string EventReaderThread::getFilePathFromFd(int fd)
{
    char buffer[PATH_MAX];

    ssize_t len;

    if (fd <= 0)
    {
        LOGWARN("Failed to get path from fd");
        return "";
    }

    std::stringstream procFdPath;
    procFdPath << "/proc/self/fd/" << fd;
    if ((len = m_sysCalls->readlink(procFdPath.str().c_str(), buffer, PATH_MAX - 1)) < 0)
    {
        LOGWARN("Failed to get path from fd: " << common::safer_strerror(errno));
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
            m_notifyPipe.notified();
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            if (!handleFanotifyEvent())
            {
                LOGDEBUG("Failed to handle Fanotify event, stopping the reading of more events");
                break;
            }
        }
    }
}
