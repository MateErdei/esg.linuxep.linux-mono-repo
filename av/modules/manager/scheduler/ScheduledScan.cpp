/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <algorithm>
#include "ScheduledScan.h"
#include "Logger.h"

using namespace manager::scheduler;

/*
 *
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>{{day}}</day>
          </daySet>
          <timeSet>
            <time>{{scheduledScanTime}}</time>
          </timeSet>
        </schedule>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>true</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>false</removableDrives>
            <kernelMemory>true</kernelMemory>
          </scanObjectSet>
          <scanBehaviour>
            <level>normal</level>
            <archives>{{scheduledScanArchives}}</archives>
            <pua>true</pua>
            <suspiciousFileDetection>false</suspiciousFileDetection>
            <scanForMacViruses>false</scanForMacViruses>
            <anti-rootkits>true</anti-rootkits>
          </scanBehaviour>
          <actions>
            <disinfect>true</disinfect>
            <puaRemoval>false</puaRemoval>
            <fileAction>doNothing</fileAction>
            <destination/>
            <suspiciousFiles>
              <fileAction>doNothing</fileAction>
              <destination/>
            </suspiciousFiles>
          </actions>
          <on-demand-options>
            <minimise-scan-impact>true</minimise-scan-impact>
          </on-demand-options>
        </settings>
      </scan>
 */

ScheduledScan::ScheduledScan()
    :
    m_valid(false),
    m_name("INVALID"),
    m_lastRunTime(static_cast<time_t>(-1))
{
}

ScheduledScan::ScheduledScan(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id)
    : m_valid(true),
      m_name(savPolicy.lookup(id + "/name").contents()),
      m_days(savPolicy, id + "/schedule/daySet/day"),
      m_times(savPolicy, id + "/schedule/timeSet/time"),
      m_lastRunTime(static_cast<time_t>(-1))
{
    m_days.sort();
    m_times.sort();
}

time_t ScheduledScan::calculateNextTime(time_t now) const
{
    if (!m_valid)
    {
        return static_cast<time_t>(-1);
    }

    struct tm now_struct{};
    struct tm* result;

    // Use localtime so that a scan runs on the basis of the local machine's timezone
    result = ::localtime_r(&now, &now_struct);
    if (result == nullptr)
    {
        LOGERROR("Failed get localtime for now");
        // Throw exception?
        return static_cast<time_t>(-1);
    }

    // First look at times, work out if the time is today or tomorrow (assuming we would run today)
    /*
     * forceTomorrow: set to true by getNextTime, if none of the time can be today
     */
    bool forceTomorrow = false;
    auto nextTime = m_times.getNextTime(now_struct, forceTomorrow);
    static_cast<void>(nextTime);

    // Now we need to work out which day
    auto nextDayDelta = m_days.getNextDay(now_struct, forceTomorrow);

    if (nextDayDelta > 0)
    {
        // Scan is not today, so we need the first time for that day
        nextTime = m_times.times().at(0);
    }

    // Re-use now_struct for the next scan execution time
    now_struct.tm_min  = nextTime.minute();
    now_struct.tm_hour = nextTime.hour();
    now_struct.tm_mday += nextDayDelta;
    now_struct.tm_sec = 0;

    return ::mktime(&now_struct);;
}
