//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ScanScheduler.h"

#include "Logger.h"
#include "ScanRunner.h"
#include "ScanSerialiser.h"

#include <common/FDUtils.h>
#include <common/SaferStrerror.h>

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <poll.h>

using namespace manager::scheduler;

const time_t INVALID_TIME = static_cast<time_t>(-1);

ScanScheduler::ScanScheduler(IScanComplete& completionNotifier)
        : m_completionNotifier(completionNotifier), m_nextScheduledScanTime(-1)
{
}

void manager::scheduler::ScanScheduler::run()
{
    announceThreadStarted();
    LOGSUPPORT("Starting scan scheduler");

    updateConfigFromPending();

    struct pollfd fds[] {
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = m_updateConfigurationPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = m_scanNowPipe.readFd(), .events = POLLIN, .revents = 0 },
    };

    while (true)
    {
        struct timespec timeout{};
        findNextTime(timeout);

        auto ret = ::ppoll(fds, std::size(fds), &timeout, nullptr);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
            break;
        }

        if (ret > 0)
        {
            if ((fds[0].revents & POLLERR) != 0)
            {
                LOGERROR("Exiting from scan scheduler, error from notify pipe");
                break;
            }

            if ((fds[1].revents & POLLERR) != 0)
            {
                LOGERROR("Exiting from scan scheduler, error from Scheduled Scan config");
                break;
            }

            if ((fds[2].revents & POLLERR) != 0)
            {
                LOGERROR("Exiting from scan scheduler, error from Scan Now config");
                break;
            }

            if ((fds[0].revents & POLLIN) != 0)
            {
                LOGSUPPORT("Exiting from scan scheduler");
                break;
            }

            if ((fds[1].revents & POLLIN) != 0)
            {
                updateConfigFromPending();
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

            if ((fds[2].revents & POLLIN) != 0)
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
            Common::Telemetry::TelemetryHelper::getInstance().increment("scheduled-scan-count", 1ul);
            runNextScan(m_nextScheduledScan);
        }
    }
    LOGDEBUG("Requesting running scans to stop");
    for (auto& item : m_runningScans)
    {
        if (item.second)
        {
            item.second->requestStop();
        }
    }
    LOGDEBUG("Waiting for running scans to stop");
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
    std::string name = nextScan.name();

    if (!nextScan.valid())
    {
        LOGERROR("Refusing to run invalid scan: " << name);
        return;
    }
    if (name.empty())
    {
        LOGERROR("Refusing to run scan with empty name!");
        return;
    }

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

bool ScanScheduler::updateConfig(manager::scheduler::ScheduledScanConfiguration config)
{
    if(config.isValid())
    {
        const std::lock_guard lock(m_pendingConfigMutex);
        m_pendingConfig = std::move(config);
        assert(m_pendingConfig.isValid());
        m_configIsPending = true;
        m_updateConfigurationPipe.notify();
        return true;
    }

    LOGWARN("Unable to accept policy as scan information is invalid. Following scans wont be run:\n" << config.scanSummary());
    return false;
}

void ScanScheduler::updateConfigFromPending()
{
    const std::lock_guard lock(m_pendingConfigMutex);
    if (m_configIsPending)
    {
        m_config = std::move(m_pendingConfig);
        assert(m_config.isValid());
        m_configIsPending = false;
    }
}