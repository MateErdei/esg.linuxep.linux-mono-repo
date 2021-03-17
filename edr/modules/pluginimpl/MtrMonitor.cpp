/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "MtrMonitor.h"

#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>

MtrMonitor::MtrMonitor(std::unique_ptr<osqueryclient::IOsqueryClient> osqueryClient) :
    m_osqueryClient(std::move(osqueryClient))
{
}

bool MtrMonitor::hasScheduledQueriesConfigured()
{
    std::string scheduledQueryCountSql = "SELECT count(name) as query_count FROM osquery_schedule;";
    auto data = queryMtr(scheduledQueryCountSql);
    if (data.has_value() && data->size() == 1)
    {
        auto row = data->back();
        if (row.find("query_count") != row.end())
        {
            LOGDEBUG("Number of MTR scheduled queries: " << row["query_count"]);
            return row["query_count"] < "5";
        }
    }
    else
    {
        LOGWARN("Cannot detect whether MTR is running scheduled queries. Defaulting to assume it is.");
    }
    // Unless we can prove MTR is not running scheduled queries then we assume it is
    return true;
}
std::optional<std::string> MtrMonitor::findMtrSocketPath()
{
    std::string mtrFlagsFilePath = Plugin::mtrFlagsFile();
    auto filesystem = Common::FileSystem::fileSystem();
    try
    {
        if (filesystem->exists(mtrFlagsFilePath))
        {
            auto flags = filesystem->readLines(mtrFlagsFilePath);
            return processFlagList(flags, "extensions_socket");
        }
    }
    catch (std::exception& ex)
    {
        LOGERROR("Failed to read MTR flags file: " << ex.what());
    }
    return std::nullopt;
}

std::optional<std::string> MtrMonitor::processFlagList(
    const std::vector<std::string>& flags,
    const std::string& flagToFind)
{
    // e.g. convert extensions_socket to --extensions_socket=
    std::string fullFlagField = "--" + flagToFind + "=";
    for (const auto& flag : flags)
    {
        size_t pos = flag.find_first_not_of(' ');
        std::string trimmedFlag = (pos == std::string::npos) ? "" : flag.substr(pos);
        if (Common::UtilityImpl::StringUtils::startswith(trimmedFlag, fullFlagField))
        {
            return trimmedFlag.substr(trimmedFlag.find(fullFlagField) + fullFlagField.length());
        }
    }
    return std::nullopt;
}

std::optional<OsquerySDK::QueryData> MtrMonitor::queryMtr(const std::string& query)
{
    if (!m_clientAlive)
    {
        m_clientAlive = connectToMtr();
    }

    if (m_clientAlive)
    {
        try
        {
            OsquerySDK::QueryData queryData;
            auto status = m_osqueryClient->query(query, queryData);
            LOGDEBUG("MTR Monitor returned query status: " << status.code);
            if (status.code == QUERY_SUCCESS)
            {
                return queryData;
            }
            else
            {
                LOGWARN("Failed to query MTR OSQuery: " << status.code << " " << status.message);
                m_clientAlive = false;
            }
        }
        catch (const std::exception& exception)
        {
            LOGWARN("Failed to query MTR OSQuery: " << exception.what());
            m_clientAlive = false;
        }
    }
    return std::nullopt;
}
bool MtrMonitor::connectToMtr()
{
    LOGDEBUG("Connecting MTR Monitor to MTR OSQuery");

    std::optional<std::string> currentMtrSocket = findMtrSocketPath();

    if (currentMtrSocket.has_value())
    {
        auto filesystem = Common::FileSystem::fileSystem();
        if (filesystem->exists(currentMtrSocket.value()))
        {
            try
            {
                m_osqueryClient->connect(currentMtrSocket.value());
                return true;
            }
            catch (const std::exception& exception)
            {
                LOGERROR("Failed to connect to MTR OSQuery: " << exception.what());
            }
        }
    }
    return false;
}
