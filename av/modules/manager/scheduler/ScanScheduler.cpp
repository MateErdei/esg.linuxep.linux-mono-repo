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

static int addFD(fd_set* fds, int fd, int currentMax)
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
        int ret = ::pselect(max+1, &tempRead, NULL, NULL, &timeout, NULL);

        if (ret < 0)
        {
            // handle error
            LOGERROR("Scheduled failed: "<< errno);
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
            }
        }

        time_t now = ::time(nullptr);
        if (now >= m_nextScanTime)
        {
            // timeout - run scan
            runNextScan();
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

void ScanScheduler::runNextScan()
{
    // serialise next scan
    std::string name = m_nextScan.name();
    std::string nextscan = serialiseNextScan();

    auto runner = std::make_unique<ScanRunner>(name, std::move(nextscan));
    runner->start();
    m_runningScans[name] = std::move(runner);
}

void ScanScheduler::updateConfig(manager::scheduler::ScheduledScanConfiguration config)
{
    m_config = std::move(config);
    m_updateConfigurationPipe.notify();
}

void ScanScheduler::findNextTime(timespec& timespec)
{
    time_t now = ::time(nullptr);
    auto next = static_cast<time_t>(-1);
    for (const auto& scan : m_config.scans())
    {
        time_t nextTime = scan.calculateNextTime(now);
        if (nextTime < next)
        {
            m_nextScan = scan;
            next = nextTime;
        }
    }
    m_nextScanTime = next;
    time_t delay = (next - now);

    // SAV halves the delay instead
    if (delay > 3600)
    {
        delay = 3600; // Only wait 1 hour, so that we can handle machine sleep/hibernate better
    }
    if (delay < 1)
    {
        // Always wait 1 second
        delay = 1;
    }
    timespec.tv_sec = delay;
    timespec.tv_nsec = 0;
}

std::string ScanScheduler::serialiseNextScan()
{
    // Move so that m_nextScan is empty
    ScheduledScan scan = std::move(m_nextScan);

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::NamedScan>();

    requestBuilder.setName(scan.name());

    auto exclusionsInput = m_config.exclusions();
    auto exclusions = requestBuilder.initExcludePaths(exclusionsInput.size());
    for (unsigned i=0; i < exclusionsInput.size(); i++)
    {
        exclusions.set(i, exclusionsInput[i]);
    }

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}
