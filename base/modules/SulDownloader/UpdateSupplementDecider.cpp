/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSupplementDecider.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <memory>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

UpdateSupplementDecider::UpdateSupplementDecider(
    UpdateSupplementDecider::WeekDayAndTimeForDelay schedule)
    : m_schedule(schedule)
{
}

time_t UpdateSupplementDecider::getLastSuccessfulProductUpdate()
{
    // Read a file from the installation
    std::string path = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLatestProductUpdateMarkerPath();
    struct stat statbuf{};
    int ret = ::stat(path.c_str(), &statbuf);
    if (ret == 0)
    {
        return statbuf.st_mtime;
    }
    return 0;
}

time_t UpdateSupplementDecider::getCurrentTime()
{
    return ::time(nullptr);
}

bool UpdateSupplementDecider::updateProducts()
{
    if (!m_schedule.enabled)
    {
        return true;
    }

    time_t lastSuccessfulProductUpdate = getLastSuccessfulProductUpdate();
    time_t lastScheduledProductUpdateTime = lastScheduledProductUpdate();

    return lastScheduledProductUpdateTime > lastSuccessfulProductUpdate;
}

time_t UpdateSupplementDecider::lastScheduledProductUpdate()
{
    time_t nowTime = getCurrentTime();
    constexpr static int SecondsInMin = 60;
    constexpr static int SecondsInHour = 60 * SecondsInMin;
    constexpr static int SecondsInDay = 24 * SecondsInHour;
    constexpr static int SecondsInWeek = 7 * SecondsInDay;

    // Work out when the last check should have happened
    std::tm now = *std::localtime(&nowTime); // assume the schedule is supposed to be local time

    int dayDiff = now.tm_wday - m_schedule.weekDay; // How many days are we past the schedule
    int hourDiff = now.tm_hour - m_schedule.hour; // How many hours are we past the schedule
    int minDiff = now.tm_min - m_schedule.minute; // How many minutes are we past the schedule

    // Total amount of seconds we are past the schedule
    time_t totalDiff = (dayDiff * SecondsInDay) + (hourDiff * SecondsInHour) + (minDiff * SecondsInMin) + now.tm_sec;

    // If the total is negative then the schedule is actually in the future for this week
    // so add SecondsInWeek to the diff, so that the scheduled time is in the past
    if (totalDiff < 0)
    {
        totalDiff += SecondsInWeek;
    }

    return nowTime - totalDiff;
}
