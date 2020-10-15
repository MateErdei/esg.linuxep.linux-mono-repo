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

    time_t nowTime = getCurrentTime();
    time_t lastSuccessfulProductUpdate = getLastSuccessfulProductUpdate();

    constexpr static int SecondsInMin = 60;
    constexpr static int SecondsInHour = 60 * SecondsInMin;
    constexpr static int SecondsInDay = 24 * SecondsInHour;
    constexpr static int SecondsInWeek = 7 * SecondsInDay;

    // Work out when the last check should have happened
    std::tm now = *std::localtime(&nowTime); // assume the schedule is supposed to be local time

    int dayDiff = now.tm_wday - m_schedule.weekDay;
    int hourDiff = now.tm_hour - m_schedule.hour;
    int minDiff = now.tm_min - m_schedule.minute;

    time_t totalDiff = (dayDiff * SecondsInDay) + (hourDiff * SecondsInHour) + (minDiff * SecondsInMin) + now.tm_sec;

    // If totalDiff is positive then it is in the future, and the actual time is a week ago.
    if (totalDiff > 0)
    {
        totalDiff -= SecondsInWeek;
    }

    time_t lastScheduledProductUpdate = nowTime - totalDiff;

    return lastScheduledProductUpdate > lastSuccessfulProductUpdate;
}
