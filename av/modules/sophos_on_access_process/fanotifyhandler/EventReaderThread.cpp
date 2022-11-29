//Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "EventReaderThread.h"
#include "Logger.h"

#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
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
    onaccessimpl::ScanRequestQueueSharedPtr scanRequestQueue,
    onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtilitySharedPtr telemetryUtility)
    : m_fanotify(std::move(fanotify))
    , m_sysCalls(std::move(sysCalls))
    , m_pluginLogDir(pluginInstall / "log" / "")
    , m_scanRequestQueue(std::move(scanRequestQueue))
    , m_telemetryUtility(std::move(telemetryUtility))
    , m_pid(getpid())
{
    assert(m_fanotify);
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path PLUGIN_INSTALL = appConfig.getData("PLUGIN_INSTALL");
    m_processExclusionStem = PLUGIN_INSTALL.string() + "/";
}

bool EventReaderThread::skipScanningOfEvent(
    struct fanotify_event_metadata* eventMetadata, std::string& filePath, std::string& exePath, int eventFd)
{
    assert(eventFd >= 0);

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
        LOGTRACE("File access on " << filePath << " will not be scanned due to log directory exclusion");
        // Result has already been logged
        std::ignore = m_fanotify->cacheFd(eventFd, filePath, true);
        return true;
    }

    exePath = get_executable_path_from_pid(eventMetadata->pid);
    if (!m_processExclusionStem.empty() && startswith(exePath, m_processExclusionStem))
    {
        //LOGDEBUG("Excluding SPL-AV process: " << executablePath << " scan type: " << eventStr << " for path: " << path);
        return true;
    }

    std::lock_guard<std::mutex> lock(m_exclusionsLock);
    for (const auto& exclusion: m_exclusions)
    {
        if (exclusion.appliesToPath(filePath))
        {
            LOGTRACE("File access on " << filePath << " will not be scanned due to exclusion: "  << exclusion.displayPath());
            // Can't cache user-created exclusions, since
            // files can be created under the exclusion then moved elsewhere.
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

    // Error only if < 0 https://man7.org/linux/man-pages/man2/read.2.html
    if (len < 0)
    {
        throwIfErrorNotRecoverable();
    }
    else if (len == 0)
    {
        LOGTRACE("Skipping empty fanotify event");
        return true;
    }

    auto* metadata = reinterpret_cast<struct fanotify_event_metadata*>(buf);

    for (; FAN_EVENT_OK(metadata, len); metadata = FAN_EVENT_NEXT(metadata, len))
    {
        if (metadata->vers != FANOTIFY_METADATA_VERSION)
        {
            LOGERROR("Fanotify wrong protocol version " << (unsigned int) metadata->vers);
            return false;
        }

        if (metadata->mask & FAN_Q_OVERFLOW)
        {
            // The in kernel event queue exceeded the limit of 16384 entries.
            LOGWARN("Fanotify queue overflowed, some files will not be scanned.");
            continue;
        }

        datatypes::AutoFd eventFd { metadata->fd };
        if (eventFd.get() < 0)
        {
            LOGERROR("Got fanotify metadata event without fd");
            continue;
        }

        std::string filePath;
        std::string executablePath;
        if (skipScanningOfEvent(metadata, filePath, executablePath, eventFd.get()))
        {
            continue;
        }

        auto uid = getUidFromPid(metadata->pid);
        std::string escapedPath;
        if (getFaNotifyHandlerLogger().isEnabledFor(log4cplus::DEBUG_LOG_LEVEL))
        {
            escapedPath = common::escapePathForLogging(filePath);
        }
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

        auto scanRequest = std::make_shared<scan_messages::ClientScanRequest>(m_sysCalls, eventFd); // DONATED

        scanRequest->setPath(filePath);
        scanRequest->setScanType(eventType);
        scanRequest->setUserID(uid);
        scanRequest->setPid(metadata->pid);
        scanRequest->setExecutablePath(executablePath);

        if (!m_scanRequestQueue->emplace(std::move(scanRequest)))
        {
            m_telemetryUtility->incrementEventReceived(true);
            if (m_EventsWhileQueueFull == 0)
            {
                LOGERROR("Failed to add scan request to queue, on-access scanning queue is full.");
            }
            m_EventsWhileQueueFull++;
        }
        else
        {
            m_telemetryUtility->incrementEventReceived(false);
            if (m_EventsWhileQueueFull > 0)
            {
                LOGINFO("Queue is no longer full. Number of events dropped: " << m_EventsWhileQueueFull);
                m_EventsWhileQueueFull = 0;
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
    try
    {
        innerRun();
    }
    catch (const std::exception& e)
    {
        LOGFATAL("EventReaderThread, Exception caught at top-level: " << e.what());
        exit(EXIT_FAILURE);
    }
    catch (...)
    {
        LOGFATAL("EventReaderThread, Non-std::exception caught at top-level");
        exit(EXIT_FAILURE);
    }
}

void EventReaderThread::innerRun()
{
    struct pollfd fds[] {
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = m_fanotify->getFd(), .events = POLLIN, .revents = 0 },
    };

    announceThreadStarted();
    m_EventsWhileQueueFull = 0;

    while (true)
    {
        int ret = m_sysCalls->ppoll(fds, std::size(fds), nullptr, nullptr);

        if (ret < 0)
        {
            if (errno != EINTR)
            {
                int error = errno;
                auto errorStr = common::safer_strerror(error);
                std::stringstream logmsg;
                logmsg << "Error from innerRun poll: " <<  error << " (" << errorStr << ")";
                throw std::runtime_error(logmsg.str());
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
                break;
            }
        }
    }
}

void EventReaderThread::setExclusions(const std::vector<common::Exclusion>& exclusions)
{
    if (exclusions != m_exclusions)
    {
        std::stringstream printableExclusions;
        for(const auto &exclusion: exclusions)
        {
            printableExclusions << "[\"" << exclusion.path().c_str() << "\"] ";
        }
        LOGDEBUG("Updating on-access exclusions with: " << printableExclusions.str());
        {
            std::lock_guard<std::mutex> lock(m_exclusionsLock);
            m_exclusions = exclusions;
        }
        // Clear cache after we have updated exclusions - so that nothing is cached which shouldn't be.
        std::ignore = m_fanotify->clearCachedFiles();
    }
}

void EventReaderThread::throwIfErrorNotRecoverable()
{
    int error = errno;
    auto errorStr = common::safer_strerror(error);

    switch (error)
    {
        case EAGAIN:
        {
            return;
        }
        case EPERM:
        case EINTR:
        case EACCES:
        {
            LOGWARN("Failed to read fanotify event, " << "(" << error << " " << errorStr << ")");
            return;
        }
        case EMFILE:
        {
            throw std::runtime_error("No more File Descriptors available. Restarting On Access");
        }
        default:
        {
            std::stringstream logmsg;
            logmsg << "Fatal Error. Restarting On Access: (" << error << " "<< errorStr << ")";
            throw std::runtime_error(logmsg.str());
        }
    }
}
