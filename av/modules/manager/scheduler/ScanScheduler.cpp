/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanScheduler.h"
#include "Logger.h"
#include "ScanRunner.h"

#include <NamedScan.capnp.h>

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace manager::scheduler;

const time_t INVALID_TIME = static_cast<time_t>(-1);

static inline bool fd_isset(int fd, fd_set* fds)
{
    assert(fd >= 0);
    return FD_ISSET(static_cast<unsigned>(fd), fds); // NOLINT
}

static inline void internal_fd_set(int fd, fd_set* fds)
{
    assert(fd >= 0);
    FD_SET(static_cast<unsigned>(fd), fds); // NOLINT
}

static int addFD(fd_set* fds, int fd, int currentMax)
{
    internal_fd_set(fd, fds);
    return std::max(fd, currentMax);
}

ScanScheduler::ScanScheduler(IScanComplete& completionNotifier)
        : m_completionNotifier(completionNotifier)
{
}

void manager::scheduler::ScanScheduler::run()
{
    announceThreadStarted();
    LOGINFO("Starting scan scheduler");

    int exitFD = m_notifyPipe.readFd();
    int configFD = m_updateConfigurationPipe.readFd();
    int scanNowFD = m_scanNowPipe.readFd();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = addFD(&readFDs, exitFD, max);
    max = addFD(&readFDs, configFD, max);
    max = addFD(&readFDs, scanNowFD, max);

    while (true)
    {
        struct timespec timeout{};
        findNextTime(timeout);

        fd_set tempRead = readFDs;
        int ret = ::pselect(max + 1, &tempRead, nullptr, nullptr, &timeout, nullptr);

        if (ret < 0)
        {
            // handle error
            LOGERROR("Scheduled failed: " << errno);
            break;
        }
        else if (ret > 0)
        {
            if (fd_isset(exitFD, &tempRead))
            {
                LOGINFO("Exiting from scan scheduler");
                break;
            }
            if (fd_isset(configFD, &tempRead))
            {
                LOGINFO("Updating scheduled scan configuration");
                while (m_updateConfigurationPipe.notified())
                {
                    // Clear updateConfigurationPipe
                }
            }
            if (fd_isset(scanNowFD, &tempRead))
            {
                LOGINFO("Starting Scan Now scan");
                runNextScan(m_config.scanNowScan());
                while (m_scanNowPipe.notified())
                {
                    // Clear scanNowPipe
                }
            }
        }

        time_t now = ::time(nullptr);
        if (now >= m_nextScheduledScanTime && m_nextScheduledScanTime != INVALID_TIME)
        {
            // timeout - run scan
            runNextScan(m_nextScheduledScan);
        }
    }
    for (auto& item : m_runningScans)
    {
        item.second->requestStop();
    }
    for (auto& item : m_runningScans)
    {
        item.second->join();
    }
    LOGINFO("Exiting scan scheduler");
}

void ScanScheduler::runNextScan(const ScheduledScan& nextScan)
{
    if (!nextScan.valid())
    {
        LOGDEBUG("Refusing to run invalid scan");
        return;
    }
    // serialise next scan
    std::string name = nextScan.name();
    std::string serialisedNextScan = serialiseNextScan(nextScan);

    auto runner = std::make_unique<ScanRunner>(name, std::move(serialisedNextScan), m_completionNotifier);
    runner->start();
    m_runningScans[name] = std::move(runner);
}

void ScanScheduler::updateConfig(manager::scheduler::ScheduledScanConfiguration config)
{
    m_config = std::move(config);
    m_updateConfigurationPipe.notify();
}

void ScanScheduler::scanNow()
{
    m_scanNowPipe.notify();
}

void ScanScheduler::findNextTime(timespec& timespec)
{
    time_t now = ::time(nullptr);
    auto next = INVALID_TIME;
    for (const auto& scan : m_config.scans())
    {
        if (!scan.valid())
        {
            continue;
        }
        time_t nextTime = scan.calculateNextTime(now);
        if (nextTime == INVALID_TIME)
        {
            continue;
        }
        if (nextTime < next)
        {
            m_nextScheduledScan = scan;
            next = nextTime;
        }
    }
    m_nextScheduledScanTime = next;
    time_t delay = (next - now);

    // SAV halves the delay instead
    if (next == INVALID_TIME || delay > 3600)
    {
        delay = 3600; // Only wait 1 hour, so that we can handle machine sleep/hibernate better
    }
    else if (delay < 1)
    {
        // Always wait 1 second
        delay = 1;
    }
    timespec.tv_sec = delay;
    timespec.tv_nsec = 0;
}

std::string ScanScheduler::serialiseNextScan(const ScheduledScan& nextScan)
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::NamedScan>();

    requestBuilder.setName(nextScan.name());
    requestBuilder.setScanArchives(nextScan.archiveScanning());
    requestBuilder.setScanAllFiles(m_config.scanAllFileExtensions());
    requestBuilder.setScanFilesWithNoExtensions(m_config.scanFilesWithNoExtensions());

    auto exclusionsInput = m_config.exclusions();
    auto exclusions = requestBuilder.initExcludePaths(exclusionsInput.size());
    for (unsigned i = 0; i < exclusionsInput.size(); i++)
    {
        exclusions.set(i, exclusionsInput[i]);
    }

    auto extensionExclusionsInput = m_config.sophosExtensionExclusions();
    auto extensionExclusions = requestBuilder.initUserDefinedExtensionInclusions(extensionExclusionsInput.size());
    for (unsigned i = 0; i < extensionExclusionsInput.size(); ++i)
    {
        extensionExclusions.set(i, extensionExclusionsInput[i]);
    }

    auto extensionInclusionsInput = m_config.userDefinedExtensionInclusions();
    auto extensionInclusions = requestBuilder.initUserDefinedExtensionInclusions(extensionInclusionsInput.size());
    for (unsigned i = 0; i < extensionInclusionsInput.size(); ++i)
    {
        extensionInclusions.set(i, extensionInclusionsInput[i]);
    }

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}
