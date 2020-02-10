/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanScheduler.h"
#include "Logger.h"
#include "ScanRunner.h"

#include "NamedScan.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace manager::scheduler;

const time_t INVALID_TIME = static_cast<time_t>(-1);

static int addFD(fd_set *fds, int fd, int currentMax)
{
    FD_SET(fd, fds);
    return std::max(fd, currentMax);
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


    while (true)
    {
        struct timespec timeout{};
        findNextTime(timeout);

        fd_set tempRead = readFDs;
        int ret = ::pselect(max + 1, &tempRead, NULL, NULL, &timeout, NULL);

        if (ret < 0)
        {
            // handle error
            LOGERROR("Scheduled failed: " << errno);
            break;
        }
        else if (ret > 0)
        {
            if (FD_ISSET(exitFD, &tempRead))
            {
                LOGINFO("Exiting from scan scheduler");
                break;
            }
            if (FD_ISSET(configFD, &tempRead))
            {
                LOGINFO("Updating scheduled scan configuration");
                while (m_updateConfigurationPipe.notified())
                {
                    // Clear updateConfigurationPipe
                }
            }
            if (FD_ISSET(scanNowFD, &tempRead))
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

    auto runner = std::make_unique<ScanRunner>(name, std::move(serialisedNextScan));
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
    if (next == INVALID_TIME)
    {
        delay = 3600;
    }
    else if (delay > 3600)
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

std::string ScanScheduler::serialiseNextScan(ScheduledScan nextScan)
{
    // Move so that nextScan is empty
    ScheduledScan scan = std::move(nextScan);

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::NamedScan>();

    requestBuilder.setName(scan.name());

    auto exclusionsInput = m_config.exclusions();
    auto exclusions = requestBuilder.initExcludePaths(exclusionsInput.size());
    for (unsigned i = 0; i < exclusionsInput.size(); i++)
    {
        exclusions.set(i, exclusionsInput[i]);
    }

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}
