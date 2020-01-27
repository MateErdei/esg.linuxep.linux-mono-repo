/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanScheduler.h"
#include "Logger.h"

using namespace manager::scheduler;

static int addFD(fd_set* fds, int fd, int currentMax)
{
    FD_SET(fd, fds);
    return std::max(fd, currentMax);
}

void manager::scheduler::ScanScheduler::run()
{
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
        else
        {
            // timeout - run scan
        }
    }
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
        next = std::min(nextTime, next);
    }
    timespec.tv_sec = (next - now);
    timespec.tv_nsec = 0;
}
