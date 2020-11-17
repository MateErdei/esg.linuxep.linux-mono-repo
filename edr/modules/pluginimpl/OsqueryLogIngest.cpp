/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryLogIngest.h"

#include "OsqueryLogger.h"
#include "TelemetryConsts.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <thirdparty/nlohmann-json/json.hpp>
void OsqueryLogIngest::ingestOutput(const std::string& output)
{
    // splitString always returns an empty string if the last char is the deliminator, i.e. a new line in this case.
    // "a;b;c;" -> { "a", "b", "c", "" }
    // This means we can assume that if we do not get an empty string as the last line in the result then we have a
    // line of text which is only partial, based on the assumption all log lines from osquery end with a new line.
    // If we have a partial line then we need to store that part in previousSection to prepend onto the next.
    std::vector<std::string> logLines =
        Common::UtilityImpl::StringUtils::splitString(m_previousSection + output, "\n");
    if (!logLines.back().empty())
    {
        m_previousSection = logLines.back();
        logLines.pop_back();
    }

    for (auto& line : logLines)
    {
        bool shouldNotLog = processOsqueryLogLineForEventsMaxTelemetry(line);
        if (shouldNotLog)
        {
            LOGINFO_OSQUERY(line);
        }

        processOsqueryLogLineForTelemetry(line);
    }
}

void OsqueryLogIngest::processOsqueryLogLineForTelemetry(std::string& logLine)
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    if (Common::UtilityImpl::StringUtils::isSubstring(
        logLine, "stopping: Maximum sustainable CPU utilization limit exceeded:"))
    {
        LOGDEBUG_OSQUERY("Increment telemetry: " << plugin::telemetryOSQueryRestartsCPU);
        telemetry.increment(plugin::telemetryOSQueryRestartsCPU, 1L);
    }
    else if (Common::UtilityImpl::StringUtils::isSubstring(logLine, "stopping: Memory limits exceeded:"))
    {
        LOGDEBUG_OSQUERY("Increment telemetry: " << plugin::telemetryOSQueryRestartsMemory);
        telemetry.increment(plugin::telemetryOSQueryRestartsMemory, 1L);
    }
    else if (Common::UtilityImpl::StringUtils::isSubstring(logLine, "Error executing scheduled query "))
    {
        //example error log: Error executing scheduled query bad_query_malformed: near "selectts": syntax error

        std::vector<std::string> sections = Common::UtilityImpl::StringUtils::splitString(logLine,"Error executing scheduled query");
        std::vector<std::string> unstrippedQueryName = Common::UtilityImpl::StringUtils::splitString(sections[1],":");
        std::string queryname = Common::UtilityImpl::StringUtils::replaceAll(unstrippedQueryName[0]," ","");

        std::stringstream key;
        key << plugin::telemetryScheduledQueries << "." << queryname << "." << plugin::telemetryQueryErrorCount;
        std::string telemetryKey = key.str();
        LOGDEBUG_OSQUERY("Increment telemetry: " << telemetryKey);
        telemetry.increment(telemetryKey, 1L);
    }

}

bool OsqueryLogIngest::processOsqueryLogLineForEventsMaxTelemetry(std::string& logLine)
{

    bool alreadySet = false;
    if (Common::UtilityImpl::StringUtils::isSubstring(logLine, "Expiring events for subscriber:"))
    {
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        std::string contents = telemetry.serialise();
        nlohmann::json array = nlohmann::json::parse(contents);
        std::string key;
        //example error log: Expiring events for subscriber: syslog_events (overflowed limit 100000)

        std::vector<std::string> sections = Common::UtilityImpl::StringUtils::splitString(logLine,"Expiring events for subscriber:");
        std::vector<std::string> unstrippedQueryName = Common::UtilityImpl::StringUtils::splitString(sections[1],"(");
        std::string tableName = Common::UtilityImpl::StringUtils::replaceAll(unstrippedQueryName[0], " ", "");

        if (tableName == "process_events")
        {
            key = plugin::telemetryProcessEventsMaxHit;
        }
        else if (tableName == "selinux_events")
        {
            key = plugin::telemetrySelinuxEventsMaxHit;
        }
        else if (tableName == "syslog_events")
        {
            key = plugin::telemetrySyslogEventsMaxHit;
        }
        else if (tableName == "user_events")
        {
            key = plugin::telemetryUserEventsMaxHit;
        }
        else if (tableName == "socket_events")
        {
            key = plugin::telemetrySocketEventsMaxHit;
        }
        else
        {
            LOGERROR_OSQUERY("Table name " << tableName << " not recognised, cannot set event_max telemetry for it");
            return alreadySet;
        }

        if (array.find(key) != array.end())
        {
            if (array[key] == true)
            {
                alreadySet = true;
            }
        }
        telemetry.set(key, true);

        LOGDEBUG_OSQUERY("Incremented telemetry: " << tableName);

    }
    return alreadySet;
}

void OsqueryLogIngest::operator()(std::string output)
{
    ingestOutput(output);
}
