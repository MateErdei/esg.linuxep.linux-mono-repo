/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryLogIngest.h"

#include "OsqueryLogger.h"
#include "TelemetryConsts.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>

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
        LOGINFO_OSQUERY(line);
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
        //Error executing scheduled query bad_query_malformed: near "selectts": syntax error
        std::vector<std::string> data= Common::UtilityImpl::StringUtils::splitString(logLine,"Error executing scheduled query");
        std::vector<std::string> data2= Common::UtilityImpl::StringUtils::splitString(data[1],":");
        std::string queryname = data2[0];
        queryname = Common::UtilityImpl::StringUtils::replaceAll(queryname," ","");
        std::stringstream key;
        key << plugin::telemetryScheduledQueries << "." << queryname << "." << plugin::telemetryQueryErrorCount;
        std::string telemetryKey = key.str();
        LOGDEBUG_OSQUERY("Increment telemetry: " << telemetryKey);
        telemetry.increment(telemetryKey, 1L);
    }

}
void OsqueryLogIngest::operator()(std::string output)
{
    ingestOutput(output);
}
