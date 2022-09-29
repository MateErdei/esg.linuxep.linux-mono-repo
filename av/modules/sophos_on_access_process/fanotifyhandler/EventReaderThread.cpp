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
#include <sys/stat.h>
#include <unistd.h>


namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::fanotifyhandler;

// Set the buffer size to the size of one memory page
static constexpr size_t FAN_BUFFER_SIZE = 4096;

EventReaderThread::EventReaderThread(
    IFanotifyHandlerSharedPtr fanotify,
    datatypes::ISystemCallWrapperSharedPtr sysCalls,
    const fs::path& pluginInstall,
    onaccessimpl::ScanRequestQueueSharedPtr scanRequestQueue)
    : m_fanotify(std::move(fanotify))
    , m_sysCalls(std::move(sysCalls))
    , m_pluginLogDir(pluginInstall / "log")
    , m_scanRequestQueue(std::move(scanRequestQueue))
    , m_pid(getpid())
{
    assert(m_fanotify);
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path PLUGIN_INSTALL = appConfig.getData("PLUGIN_INSTALL");
    m_processExclusionStem = PLUGIN_INSTALL.string() + "/";
}

bool EventReaderThread::skipScanningOfEvent(
    struct fanotify_event_metadata* eventMetadata, std::string& filePath, std::string& exePath)
{
    if (eventMetadata->vers != FANOTIFY_METADATA_VERSION)
    {
        LOGERROR("fanotify wrong protocol version " << eventMetadata->vers);
        return true;
    }

    auto eventFd = eventMetadata->fd;
    if (eventFd < 0)
    {
        LOGDEBUG("Got fanotify metadata event without fd");
        return true;
    }

    if (eventMetadata->pid == m_pid)
    {
        return true;
    }

    filePath = getFilePathFromFd(eventFd);
    //Either path was too long or fd was invalid
    if(filePath.empty())
    {
        return true;
    }

    // Exclude events caused by AV logging to prevent recursive events
    if (filePath.rfind(m_pluginLogDir, 0) == 0)
    {
        return true;
    }

    exePath = get_executable_path_from_pid(eventMetadata->pid);
    if (!m_processExclusionStem.empty() && startswith(exePath, m_processExclusionStem))
    {
        //LOGDEBUG("Excluding SPL-AV process: " << executablePath << " scantype: " << eventStr << " for path: " << path);
        return true;
    }

    for (const auto& exclusion: m_exclusions)
    {
        if (exclusion.appliesToPath(filePath))
        {
            LOGTRACE("File access on " << filePath << " will not be scanned due to exclusion: "  << exclusion.displayPath());
            return true;
        }
    }
    return false;
}

bool EventReaderThread::handleFanotifyEvent()
{
    char buf[FAN_BUFFER_SIZE];

    errno = 0;
    ssize_t len = m_sysCalls->read(m_fanotify->getFd(), buf, sizeof(buf));

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

    auto* metadata = reinterpret_cast<struct fanotify_event_metadata*>(buf);

    for (; FAN_EVENT_OK(metadata, len); metadata = FAN_EVENT_NEXT(metadata, len))
    {
        std::string filePath;
        std::string executablePath;
        if (skipScanningOfEvent(metadata, filePath, executablePath))
        {
            continue;
        }

        auto uid = getUidFromPid(metadata->pid);
        auto escapedPath = common::escapePathForLogging(filePath);

        auto eventType = E_SCAN_TYPE_UNKNOWN;

        //Some events have both bits set, we prioritise FAN_CLOSE_WRITE as the event tag. A copy event can cause this.
        if ((metadata->mask & FAN_CLOSE_WRITE) && (metadata->mask & FAN_OPEN))
        {
            LOGDEBUG("On-open event for " << escapedPath << " from Process " << executablePath << "(PID=" << metadata->pid << ") " << "and UID " << uid);
            LOGDEBUG("On-close event for " << escapedPath << " from Process " << executablePath << "(PID=" << metadata->pid << ") " << "and UID " << uid);
            eventType = E_SCAN_TYPE_ON_ACCESS_CLOSE;
        }
        else if (metadata->mask & FAN_CLOSE_WRITE)
        {
            LOGDEBUG("On-close event for " << escapedPath << " from Process " << executablePath << "(PID=" << metadata->pid << ") " << "and UID " << uid);
            eventType = E_SCAN_TYPE_ON_ACCESS_CLOSE;
        }
        else if (metadata->mask & FAN_OPEN)
        {
            LOGDEBUG("On-open event for " << escapedPath << " from Process " << executablePath << "(PID=" << metadata->pid << ") " << "and UID " << uid);
            eventType = E_SCAN_TYPE_ON_ACCESS_OPEN;
        }
        else
        {
            LOGERROR("unknown operation mask: " << std::hex << metadata->mask << std::dec);
            continue;
        }

        auto scanRequest = std::make_shared<scan_messages::ClientScanRequest>();
        scanRequest->setFd(metadata->fd); // DONATED
        scanRequest->setPath(filePath);
        scanRequest->setScanType(eventType);
        scanRequest->setUserID(uid);

        if (!m_scanRequestQueue->emplace(std::move(scanRequest)))
        {
            LOGERROR("Failed to add scan request to queue, on-access scanning queue is full. Path will not be scanned: " << filePath);
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
        return "unknown";
    }

    std::stringstream procFdPath;
    procFdPath << "/proc/self/fd/" << fd;
    if ((len = m_sysCalls->readlink(procFdPath.str().c_str(), buffer, PATH_MAX - 1)) < 0)
    {
        LOGWARN("Failed to get path from fd: " << common::safer_strerror(errno));
        return "unknown";
    }

    buffer[len] = '\0';
    return buffer;
}

std::string EventReaderThread::getUidFromPid(pid_t pid)
{
    std::stringstream procPidPath;
    procPidPath << "/proc/" << pid;
    struct ::stat statbuf{};

    int ret = m_sysCalls->_stat(procPidPath.str().c_str(), &statbuf);
    if (ret == 0)
    {
        return std::to_string(statbuf.st_uid);
    }
    return "unknown";
}

void EventReaderThread::run()
{
    struct pollfd fds[] {
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = m_fanotify->getFd(), .events = POLLIN, .revents = 0 },
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

void EventReaderThread::setExclusions(std::vector<common::Exclusion> exclusions)
{
    if (exclusions != m_exclusions)
    {
        LOGDEBUG("Updating on-access exclusions");
        m_exclusions = exclusions;
    }
}
