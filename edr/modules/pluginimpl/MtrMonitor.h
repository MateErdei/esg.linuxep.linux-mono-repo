/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once

#include <modules/osqueryclient/OsqueryClientImpl.h>
class MtrMonitor
{
public:
    MtrMonitor(std::unique_ptr<osqueryclient::IOsqueryClient> osqueryClient);
    bool hasScheduledQueriesConfigured();

protected:
    static const int QUERY_SUCCESS = 0;
    std::unique_ptr<osqueryclient::IOsqueryClient> m_osqueryClient;
    std::optional<std::string> m_currentMtrSocket = std::nullopt;
    bool m_clientAlive = false;

    // flags is a vector of strings of the form --flag=value from the osquery flags file.
    // flagToFind does not include the hyphens
    std::optional<std::string> processFlagList(const std::vector<std::string>& flags, const std::string& flagToFind);
    std::optional<std::string> findMtrSocketPath();
    std::optional<OsquerySDK::QueryData> queryMtr(const std::string& query);
    bool connectToMtr();
};

