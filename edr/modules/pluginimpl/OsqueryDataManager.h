/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/UtilityImpl/TimeUtils.h>
#include <OsquerySDK/OsquerySDK.h>
#include <osqueryclient/IOsqueryClient.h>
#include "PluginUtils.h"
#include <future>

struct OsqueryDataRetentionCheckState
{
    bool running = false;
    int numberOfRetries = 5;
    bool firstRun = true;
    std::chrono::steady_clock::time_point lastOSQueryDataCheck = std::chrono::steady_clock::now();
    const std::chrono::duration<int64_t, std::ratio<60>> osqueryDataCheckPeriod = std::chrono::minutes(30);
};

class OsqueryDataManager
{
public:
    OsqueryDataManager();
    void cleanUpOsqueryLogs();
    void removeOldWarningFiles();
    void rotateFiles(const std::string& path, int limit);

    void reconfigureDataRetentionParameters(unsigned long currentEpochTime, unsigned long oldestRecordTime);
    unsigned long getOldestAllowedTimeForCurrentEventedData();
    void asyncCheckAndReconfigureDataRetention(std::shared_ptr<OsqueryDataRetentionCheckState> osqueryDataRetentionCheckState);
    void purgeDatabase();
    std::string buildLimitQueryString(unsigned int limit);

private:
    std::optional<OsquerySDK::QueryData> runQuery(const std::string& query);
    bool connectToOsquery();

    std::future<void> m_dataCheckThreadMonitor;
    static const int QUERY_SUCCESS = 0;
    std::unique_ptr<osqueryclient::IOsqueryClient> m_osqueryClient;
    bool m_clientAlive = false;
    const unsigned int MAX_LOGFILE_SIZE = 1024 * 1024;
    const unsigned int FILE_LIMIT = 10;
    const unsigned long MAX_EVENTED_DATA_RETENTION_TIME = 604800; // 7 days in seconds.

    const unsigned long EVENT_MAX_DEFAULT = Plugin::PluginUtils::MAXIMUM_EVENTED_RECORDS_ALLOWED; // 100k events
    const unsigned long EVENT_MAX_LOWER_LIMIT = 1; // 1 event
    const unsigned long EVENT_MAX_UPPER_LIMIT = 1000000; // 1M events
    unsigned long m_eventsMaxFromConfigAtStart;
};

