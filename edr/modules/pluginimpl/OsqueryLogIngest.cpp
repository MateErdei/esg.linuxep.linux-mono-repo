// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "OsqueryLogIngest.h"
#include "OsqueryLogStringUtil.h"
#include "Logger.h"
#include "OsqueryLogger.h"
#include  "ScheduledQueryLogger.h"
#include "EdrCommon/TelemetryConsts.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <nlohmann/json.hpp>

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
        bool alreadyLogged = processOsqueryLogLineForEventsMaxTelemetry(line);
        if (!alreadyLogged)
        {
            std::optional<std::string> scheduledQueryLogLine = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(line);
            if(scheduledQueryLogLine.has_value())
            {
                LOGINFO_SCHEDULEDOSQUERY(scheduledQueryLogLine.value());
            }

            if(OsqueryLogStringUtil::isGenericLogLine(line))
            {
                LOGINFO(line);
            }

            // The below lines are checks that indicate the reason for OSQuery restarting
            // Want to log them at INFO level as they are important
            std::string linesToAlwaysLog[] {
                // Other OSQuery restarts
                "An error occurred during extension manager startup",
                "Refreshing configuration state",

                // CPU OSQuery restart
                "stopping: Maximum sustainable CPU utilization limit",

                // Memory OSQuery restarts
                "Memory limits exceeded",
            };

            bool matchFound = false;
            for (auto& logLineCheck : linesToAlwaysLog)
            {
                if (line.find(logLineCheck) != std::string::npos)
                {
                    LOGWARN_OSQUERY(line);
                    matchFound = true;
                    break;
                }
            }
            if (!matchFound)
            {
                LOGDEBUG_OSQUERY(line);
            }
        }

        processOsqueryLogLineForTelemetry(line);
    }
}

void OsqueryLogIngest::processOsqueryLogLineForTelemetry(std::string& logLine)
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    if (Common::UtilityImpl::StringUtils::isSubstring(
        logLine, "stopping: Maximum sustainable CPU utilization limit"))
    {
        LOGDEBUG("Increment telemetry: " << plugin::telemetryOSQueryRestartsCPU);
        telemetry.increment(plugin::telemetryOSQueryRestartsCPU, 1L);
    }
    else if (Common::UtilityImpl::StringUtils::isSubstring(logLine, "stopping: Memory limits exceeded:"))
    {
        LOGDEBUG("Increment telemetry: " << plugin::telemetryOSQueryRestartsMemory);
        telemetry.increment(plugin::telemetryOSQueryRestartsMemory, 1L);
    }
    else if (Common::UtilityImpl::StringUtils::isSubstring(logLine, "Refreshing configuration state"))
    {
        LOGDEBUG("Increment telemetry: " << plugin::telemetryOSQueryRestartsConfigRefresh);
        telemetry.increment(plugin::telemetryOSQueryRestartsConfigRefresh, 1L);
    }
    else if (Common::UtilityImpl::StringUtils::isSubstring(logLine, "An error occurred during extension manager startup"))
    {
        LOGDEBUG("Increment telemetry: " << plugin::telemetryOSQueryRestartsExtMgrError);
        telemetry.increment(plugin::telemetryOSQueryRestartsExtMgrError, 1L);
    }
}

bool OsqueryLogIngest::processOsqueryLogLineForEventsMaxTelemetry(std::string& logLine)
{

    bool alreadyLogged = false;
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
            LOGERROR("Table name " << tableName << " not recognised, cannot set event_max telemetry for it");
            return alreadyLogged;
        }

        if (array.find(key) != array.end())
        {
            if (array[key] == true)
            {
                alreadyLogged = true;
            }
        }
        telemetry.set(key, true);

        LOGDEBUG("Setting true for telemetry key: " << key);

    }
    return alreadyLogged;
}

void OsqueryLogIngest::operator()(std::string output)
{
    ingestOutput(output);
}
