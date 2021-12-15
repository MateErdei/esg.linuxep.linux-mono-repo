/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/UtilityImpl/TimeUtils.h>
#include <OsquerySDK/OsquerySDK.h>
#include <osqueryclient/IOsqueryClient.h>

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
    void cleanUpOsqueryLogs();
    void removeOldWarningFiles();
    void rotateFiles(std::string path, int limit);

    void reconfigureDataRetentionParameters(unsigned long currentEpochTime, unsigned long oldestRecordTime);
    unsigned long getOldestAllowedTimeForCurrentEventedData();
    void asyncCheckAndReconfigureDataRetention(std::shared_ptr<OsqueryDataRetentionCheckState> osqueryDataRetentionCheckState);
    void purgeDatabase();
private:
    std::optional<OsquerySDK::QueryData> runQuery(const std::string& query);
    bool connectToOsquery();

    static const int QUERY_SUCCESS = 0;
    std::unique_ptr<osqueryclient::IOsqueryClient> m_osqueryClient;
    bool m_clientAlive = false;
    const unsigned int MAX_LOGFILE_SIZE = 1024 * 1024;
    const unsigned int FILE_LIMIT = 10;
    const unsigned long MAX_EVENTED_DATA_RETENTION_TIME = 604800; // 7 days in seconds.

};

