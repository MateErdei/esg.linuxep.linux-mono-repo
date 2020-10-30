/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanScheduler.h"
#include "Logger.h"
#include "ScanRunner.h"
#include "ScanSerialiser.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
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
        : m_completionNotifier(completionNotifier), m_nextScheduledScanTime(-1)
{
}

void manager::scheduler::ScanScheduler::run()
{
    announceThreadStarted();

    LOGSUPPORT("Starting scan scheduler");

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
            LOGERROR("Failed to start scheduled scan: " << errno);
            break;
        }
        else if (ret > 0)
        {
            if (fd_isset(exitFD, &tempRead))
            {
                LOGSUPPORT("Exiting from scan scheduler");
                break;
            }
            if (fd_isset(configFD, &tempRead))
            {
                LOGINFO("Configured number of Scheduled Scans: " << m_config.scans().size());
                for (const auto& scan : m_config.scans() )
                {
                    LOGINFO(scan.str());
                }
                LOGINFO("Configured number of Exclusions: " << m_config.exclusions().size());
                LOGINFO("Configured number of Sophos Defined Extension Exclusions: " << m_config.sophosExtensionExclusions().size());
                LOGINFO("Configured number of User Defined Extension Exclusions: " << m_config.userDefinedExtensionInclusions().size());

                while (m_updateConfigurationPipe.notified())
                {
                    // Clear updateConfigurationPipe
                }
            }
            if (fd_isset(scanNowFD, &tempRead))
            {
                LOGINFO("Evaluating Scan Now");
                while (m_scanNowPipe.notified())
                {
                    // Clear scanNowPipe before starting the scan, otherwise we might miss notifications arriving
                    // after we've started the scan.
                }
                Common::Telemetry::TelemetryHelper::getInstance().increment("scan-now-count", 1ul);
                runNextScan(m_config.scanNowScan());
            }
        }

        time_t now = get_current_time(false);
        if (now >= m_nextScheduledScanTime && m_nextScheduledScanTime != INVALID_TIME)
        {
            // timeout - run scan
            runNextScan(m_nextScheduledScan);
        }
    }
    for (auto& item : m_runningScans)
    {
        if (item.second)
        {
            item.second->requestStop();
        }
    }
    for (auto& item : m_runningScans)
    {
        if (item.second)
        {
            item.second->join();
        }
    }
    LOGSUPPORT("Exiting scan scheduler");
}

void ScanScheduler::runNextScan(const ScheduledScan& nextScan)
{
    if (!nextScan.valid())
    {
        LOGERROR("Refusing to run invalid scan: " << nextScan.name());
        return;
    }

    std::string name = nextScan.name();
    bool already_running = false;
    auto it = m_runningScans.begin();

    while (it != m_runningScans.end())
    {
        if (!it->second || it->second->scanCompleted())
        {
            // Scan completed
            it = m_runningScans.erase(it);
            continue;
        }
        else if (it->first == name)
        {
            // existing scan of the same name running
            already_running = true;
        }
        ++it;
    }

    if (already_running)
    {
        LOGWARN("Refusing to run a second Scan named: " << name);
        return;
    }

    // serialise next scan
    std::string serialisedNextScan = serialiseNextScan(nextScan);

    auto runner = std::make_unique<ScanRunner>(name, std::move(serialisedNextScan), m_completionNotifier);
    assert(runner);
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
    time_t now = get_current_time(true);
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
        if (next == INVALID_TIME || nextTime < next)
        {
            m_nextScheduledScan = scan;
            next = nextTime;
        }
    }
    m_nextScheduledScanTime = next;
    timespec.tv_sec = calculate_delay(next, now);
    timespec.tv_nsec = 0;
}

time_t ScanScheduler::calculate_delay(time_t next_scheduled_scan_time, time_t now)
{
    time_t delay = (next_scheduled_scan_time - now);

    // SAV halves the delay instead
    if (next_scheduled_scan_time == INVALID_TIME || delay > 3600)
    {
        delay = 3600; // Only wait 1 hour, so that we can handle machine sleep/hibernate better
    }
    else if (delay < 1)
    {
        // Always wait 1 second
        delay = 1;
    }
    return delay;
}


std::string ScanScheduler::serialiseNextScan(const ScheduledScan& nextScan)
{
    return ScanSerialiser::serialiseScan(m_config, nextScan);
}
time_t ScanScheduler::get_current_time(bool)
{
    return ::time(nullptr);
}