// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "OsqueryLogStringUtil.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"

std::optional<std::string> OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(const std::string& logLine)
{
    std::string lineToFind = "Executing scheduled query ";
    if (Common::UtilityImpl::StringUtils::isSubstring(logLine, lineToFind))
    {
        std::vector<std::string> items = Common::UtilityImpl::StringUtils::splitString(logLine, lineToFind);
        if (items.size() < 2)
        {
            return {};
        }

        auto first = items[0].find_first_not_of(" ");
        if (first == std::string::npos)
        {
            return {};
        }
        std::string time;
        std::vector<std::string> timeString = Common::UtilityImpl::StringUtils::splitString(items[0].substr(first), " ");
        if (timeString.size() < 2)
        {
            return {};
        }
        else
        {
            time = timeString[1];
        }

        std::vector<std::string> nameString = Common::UtilityImpl::StringUtils::splitString(items[1], ":");

        std::string queryName;

        if (nameString.empty() || time.empty() || nameString[0].empty())
        {
            return {};
        }
        else
        {
            queryName = nameString[0];
        }
        std::vector<std::string> listOfFlushQueries= {"flush_process_events","flush_user_events","flush_selinux_events","flush_syslog_events"};
        if (std::find(std::begin(listOfFlushQueries), std::end(listOfFlushQueries), queryName) != std::end(listOfFlushQueries))
        {
            return {};
        }
        std::stringstream lineReturned;
        lineReturned << "Executing query: " << queryName;
        return lineReturned.str();
    }
    return {};
}

bool OsqueryLogStringUtil::isGenericLogLine(const std::string& logLine)
{
    std::vector<std::string> interestingLines { "Error executing", "failed to parse config" };

    for (auto& interestingLine : interestingLines)
    {
        if (Common::UtilityImpl::StringUtils::isSubstring(logLine, interestingLine))
        {
            return true;
        }
    }
    return false;
}
