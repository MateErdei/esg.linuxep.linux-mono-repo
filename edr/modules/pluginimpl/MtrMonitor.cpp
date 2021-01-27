/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "MtrMonitor.h"

#include "ApplicationPaths.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>

MtrMonitor::MtrMonitor() : m_currentMtrSocket(findMtrSocketPath())
{
    connectToMtr();
}

bool MtrMonitor::hasScheduledQueriesConfigured()
{
    // to do query MTR
    //m_osqueryClient.query()
    std::string scheduledQueryCountSql = "SELECT count(*) FROM osquery_schedule;";
    auto data = queryMtr(scheduledQueryCountSql);
    if (data.has_value())
    {
        // loop through data for the key we want.
        // then return value
    }

    return false;
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
        // handle error reading file...
    }
    return std::nullopt;
}

std::optional<std::string> MtrMonitor::processFlagList(const std::vector<std::string>& flags, const std::string& flagToFind)
{
    // e.g. convert extensions_socket to --extensions_socket=
    std::string fullFlagLhs = "--" + flagToFind + "=";
    for (const auto& flag : flags)
    {
        if (Common::UtilityImpl::StringUtils::startswith(flag, fullFlagLhs))
        {
            // then we found the flag
            return Common::UtilityImpl::StringUtils::splitString(flag, "=").back();
        }
    }
    return std::nullopt;
}

std::optional<OsquerySDK::QueryData> MtrMonitor::queryMtr(const std::string& query)
{
    if (!m_currentMtrSocket.has_value())
    {
        // Try to find socket here if we can.
        m_currentMtrSocket = findMtrSocketPath();
    }

    if (m_currentMtrSocket.has_value())
    {
        OsquerySDK::QueryData queryData;
        auto status = m_osqueryClient.query(query, queryData);
        if (status.code == QUERY_SUCCESS)
        {
            return queryData;
        }
    }
    // if query failed then return unset optional
    return std::nullopt;
}
void MtrMonitor::connectToMtr()
{

    // todo this bit is dupe code, tidy
    if (!m_currentMtrSocket.has_value())
    {
        // Try to find socket here if we can.
        m_currentMtrSocket = findMtrSocketPath();
    }
    /////////


    if (!m_currentMtrSocket.has_value())
    {
        return;
    }

    try
    {
        m_osqueryClient.connect(m_currentMtrSocket.value());
    }
    catch (const std::exception& exception)
    {
        // TODO handle error
    }
}
