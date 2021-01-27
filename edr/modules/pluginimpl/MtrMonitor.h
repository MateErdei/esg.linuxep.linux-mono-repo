/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once

//#include <modules/osqueryclient/IOsqueryClient.h>
#include <modules/osqueryclient/OsqueryClientImpl.h>
class MtrMonitor
{
public:
    MtrMonitor();
    bool hasScheduledQueriesConfigured();

private:
    static const int QUERY_SUCCESS = 0;
    osqueryclient::OsqueryClientImpl m_osqueryClient;
    std::optional<std::string> m_currentMtrSocket;

    // flags is a vector of strings of the form --flag=value from the osquery flags file.
    std::optional<std::string> processFlagList(const std::vector<std::string>& flags, const std::string& flagToFind);
    std::optional<std::string> findMtrSocketPath();
    std::optional<OsquerySDK::QueryData> queryMtr(const std::string& query);
    void connectToMtr();
};

