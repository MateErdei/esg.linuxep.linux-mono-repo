// Copyright 2022 Sophos Limited. All rights reserved.

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
    onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtilitySharedPtr telemetryUtility,
    mount_monitor::mountinfo::IDeviceUtilSharedPtr deviceUtil)
    : m_fanotify(std::move(fanotify))
    , m_sysCalls(std::move(sysCalls))
    , m_pluginLogDir(pluginInstall / "log" / "")
    , m_scanRequestQueue(std::move(scanRequestQueue))
    , m_telemetryUtility(std::move(telemetryUtility))
    , m_deviceUtil(std::move(deviceUtil))
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
        // Don't ever get the file-path - which is only used when reporting errors
        // At least for the investigation, don't change this...
        if (m_cacheAllEvents)
        {
            std::ignore = m_fanotify->cacheFd(eventFd, "", false);
        }
        return true;
    }

    filePath = getFilePathFromFd(eventFd);
    if (m_cacheAllEvents)
    {
        std::ignore = m_fanotify->cacheFd(eventFd, filePath, false);
    }
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
    else
    {
        // Got a successful read
        m_readFailureCount = 0;
    }

    if (len == 0)
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

        auto scanRequest = std::make_shared<scan_request_t>(m_sysCalls, eventFd); // DONATED
        scanRequest->setPath(filePath);
        scanRequest->setScanType(eventType);
        scanRequest->setUserID(uid);
        scanRequest->setPid(metadata->pid);
        scanRequest->setExecutablePath(executablePath);
        if (m_cacheAllEvents)
        {
            scanRequest->setIsCached(true);
        }

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

/**
 * Fanotify read() calls can return various errno, so we have several approaches to deal with them
 */
void EventReaderThread::throwIfErrorNotRecoverable()
{
    int error = errno;
    auto errorStr = common::safer_strerror(error);

    /*
     *
     * We split errno into 4 categories:
     * 1) no-log immediate retry: signals and non-blocking return
     * 2) unlisted errno, probably from fuse filesystem: Log and retry, throw exception if too many in a row
     * 3) recoverable errno: Log and retry after delay, throw exception if too many in a row
     * 4) Fatal errno: Log and throw exception - generally indicate fanotify fd is invalid
     *
     * Unexpected/unknown errno are treated as 3)
     */

    /*
     * https://man7.org/linux/man-pages/man7/fanotify.7.html
     *
     *
*  If a call to read(2) processes multiple events from the
   fanotify queue and an error occurs, the return value will be
   the total length of the events successfully copied to the
   user-space buffer before the error occurred.  The return value
   will not be -1, and errno will not be set.  Thus, the reading
   application has no way to detect the error.
     *
     * fanotify read errnos:
     *
EINVAL The buffer is too small to hold the event.
Fatal: Not recoverable by current code - buffer is fixed size

EMFILE The per-process limit on the number of open files has been
       reached.  See the description of RLIMIT_NOFILE in
       getrlimit(2).
Retry: Can retry and hope some fd have been released
Log error; wait 100ms; limited retries

ENFILE The system-wide limit on the total number of open files
       has been reached.  See /proc/sys/fs/file-max in proc(5).
Retry: Can retry and hope some fd have been released
Log error; wait 100ms; limited retries

ETXTBSY
       This error is returned by read(2) if O_RDWR or O_WRONLY
       was specified in the event_f_flags argument when calling
       fanotify_init(2) and an event occurred for a monitored
       file that is currently being executed.
Fatal: We don't set those flags to fanotify_init

       * general read errnos:
       *
EAGAIN The file descriptor fd refers to a file other than a
       socket and has been marked nonblocking (O_NONBLOCK), and
       the read would block.  See open(2) for further details on
       the O_NONBLOCK flag.
Return: Could be O_NONBLOCK in future

EAGAIN or EWOULDBLOCK
       The file descriptor fd refers to a socket and has been
       marked nonblocking (O_NONBLOCK), and the read would block.
       POSIX.1-2001 allows either error to be returned for this
       case, and does not require these constants to have the
       same value, so a portable application should check for
       both possibilities.
Return: Could be O_NONBLOCK in future

EBADF  fd is not a valid file descriptor or is not open for
       reading.
Fatal: Fanotify descriptor is no longer valid

EFAULT buf is outside your accessible address space.
Fatal: Stack buffer is no longer valid

EINTR  The call was interrupted by a signal before any data was
       read; see signal(7).
Retry

EINVAL fd is attached to an object which is unsuitable for
       reading; or the file was opened with the O_DIRECT flag,
       and either the address specified in buf, the value
       specified in count, or the file offset is not suitably
       aligned.
Fatal: fanotify descriptor replaced

EINVAL fd was created via a call to timerfd_create(2) and the
       wrong size buffer was given to read(); see
       timerfd_create(2) for further information.
Fatal: As above

EIO    I/O error.  This will happen for example when the process
       is in a background process group, tries to read from its
       controlling terminal, and either it is ignoring or
       blocking SIGTTIN or its process group is orphaned.  It may
       also occur when there is a low-level I/O error while
       reading from a disk or tape.  A further possible cause of
       EIO on networked filesystems is when an advisory lock had
       been taken out on the file descriptor and this lock has
       been lost.  See the Lost locks section of fcntl(2) for
       further details.
Fatal: fanotify descriptor replaced??

EISDIR fd refers to a directory.
Fatal: fanotify descriptor replaced


 Other Errno observed but not listed:
EPERM  virtualbox host filesystem LINUXDAR-6262
Log warning and limited retries

EACCES  ???
Log warning and limited retries

ENOENT  fuse?

     */

    std::stringstream logmsg;
    switch (error)
    {
        case EINTR:
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
        {
            // Case 1
            // Expected, no warning required,
            // return to ppoll loop so that we don't get stuck in blocking read.
            break;
        }
        case EPERM:
        case EACCES:
        case ENOENT: // Only listing on fanotify(7) is for write calls.
        {
            // Case 2
            // Errors from e.g. fuse filesystems.
            // We assume these will immediately 'recover', throwing away the event from fuse
            LOGWARN("Failed to read fanotify event, " << "(" << error << " " << errorStr << ")");
            m_readFailureCount++;
            if (m_readFailureCount > RESTART_SOAP_ERROR_COUNT)
            {
                logmsg << "Too many EPERM/EACCES/ENOENT errors. Restarting On Access: (" << error << " "<< errorStr << ")";
                throw std::runtime_error(logmsg.str());
            }
            break;
        }
        case EINVAL:
        case ETXTBSY:
        case EBADF:
        case EIO:
        case EISDIR:
        {
            // Case 4
            // immediately fatal - we have no way of recovering from this
            LOGFATAL("Failed to read fanotify event, " << "(" << error << " " << errorStr << ")");
            logmsg << "Fatal Error. Restarting On Access: (" << error << " "<< errorStr << ")";
            throw std::runtime_error(logmsg.str());
        }
        case EMFILE:
        case ENFILE:
        {
            // Case 3
            // If we run out of file descriptors, then do a delay and retry
            // This should allow other threads/processes to free descriptors
            LOGERROR("Failed to read fanotify event. No more File Descriptors available: " << "(" << error << " " << errorStr << ")");
            m_readFailureCount++;

            if (m_readFailureCount > RESTART_SOAP_ERROR_COUNT)
            {
                throw std::runtime_error("No more File Descriptors available. Restarting On Access");
            }
            else
            {
                // a small sleep to allow FD to be released
                std::this_thread::sleep_for(m_out_of_file_descriptor_delay);
            }
            break;
        }
        default:
        {
            // If we get an unknown errno, then we just do the delay and retry approach (case 3)
            LOGERROR("Failed to read fanotify event. Unexpected error available: " << "(" << error << " " << errorStr << ")");
            m_readFailureCount++;

            if (m_readFailureCount > RESTART_SOAP_ERROR_COUNT)
            {
                logmsg << "Failed to read fanotify event: Fatal Error. Restarting On Access: (" << error << " "<< errorStr << ")";
                throw std::runtime_error(logmsg.str());
            }
            else
            {
                // a small sleep to allow FD to be released
                std::this_thread::sleep_for(m_out_of_file_descriptor_delay);
            }
            break;
        }
    }
}

void EventReaderThread::setCacheAllEvents(bool enable)
{
    m_cacheAllEvents = enable;
}
