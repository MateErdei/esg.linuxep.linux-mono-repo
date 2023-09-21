// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "OsqueryDataManager.h"

#include "ApplicationPaths.h"
#include "Logger.h"
#include "OsqueryConfigurator.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/WaitForUtils.h>

void OsqueryDataManager::cleanUpOsqueryLogs()
{
    std::string logPath = Plugin::osQueryResultsLogPath();
    auto* ifileSystem = Common::FileSystem::fileSystem();
    try
    {
        off_t size = ifileSystem->fileSize(logPath);

        if (size > MAX_LOGFILE_SIZE)
        {
            LOGDEBUG("Rotating osquery logs");
            std::string fileToDelete = logPath + ".10";

            if (ifileSystem->isFile(fileToDelete))
            {
                LOGINFO("Log limit reached : Deleting oldest osquery log file");
                ifileSystem->removeFile(fileToDelete);
            }

            rotateFiles(logPath, 9);

            ifileSystem->moveFile(logPath, logPath + ".1");
        }
        removeOldWarningFiles();
    }
    catch (Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Failed to clean up old osquery files with error: " << ex.what());
    }
}
void OsqueryDataManager::removeOldWarningFiles()
{
    LOGDEBUG("Checking for osquery INFO/WARNING files");
    auto* ifileSystem = Common::FileSystem::fileSystem();
    std::vector<std::string> files = ifileSystem->listFiles(Plugin::osQueryLogDirectoryPath());
    std::vector<std::string> warningFiles;
    std::vector<std::string> infoFiles;
    try
    {
        for (const auto& file : files)
        {
            std::string filename = Common::FileSystem::basename(file);
            if (filename.find("osqueryd.WARNING.") != std::string::npos)
            {
                warningFiles.push_back(file);
            }
            else if (filename.find("osqueryd.INFO.") != std::string::npos)
            {
                infoFiles.push_back(file);
            }
        }

        if (infoFiles.size() > FILE_LIMIT)
        {
            std::sort(infoFiles.begin(), infoFiles.end());

            unsigned int infoFilesToDelete = infoFiles.size() - FILE_LIMIT;

            for (unsigned int i = 0; i < infoFilesToDelete; i++)
            {
                ifileSystem->removeFile(infoFiles[i]);
                LOGINFO("Removed old osquery INFO file: " << Common::FileSystem::basename(infoFiles[i]));
            }
        }

        if (warningFiles.size() > FILE_LIMIT)
        {
            std::sort(warningFiles.begin(), warningFiles.end());

            unsigned int warningFilesToDelete = warningFiles.size() - FILE_LIMIT;
            for (unsigned int i = 0; i < warningFilesToDelete; i++)
            {
                ifileSystem->removeFile(warningFiles[i]);
                LOGINFO("Removed old osquery WARNING file: " << Common::FileSystem::basename(warningFiles[i]));
            }
        }
    }
    catch (Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Failed to clean up old osquery WARNING/INFO files with error: " << ex.what());
    }
}

void OsqueryDataManager::rotateFiles(const std::string& path, int limit)
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    int iterator = limit;
    try
    {
        while (iterator > 0)
        {
            std::string oldExtension = "." + std::to_string(iterator);
            std::string fileToIncrement = path + oldExtension;

            if (ifileSystem->isFile(fileToIncrement))
            {
                std::string newExtension = "." + std::to_string(iterator + 1);
                std::string fileDestination = path + newExtension;
                ifileSystem->moveFile(fileToIncrement, fileDestination);
            }

            iterator -= 1;
        }
    }
    catch (Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Failed to rotate files with error: " << ex.what());
    }
}

void OsqueryDataManager::reconfigureDataRetentionParameters(unsigned long currentEpochTime, unsigned long oldestRecordTime)
{
    unsigned long newRetentionDataTimeInSeconds = MAX_EVENTED_DATA_RETENTION_TIME;

    if ((oldestRecordTime != MAX_EVENTED_DATA_RETENTION_TIME) &&
        (currentEpochTime > oldestRecordTime) &&
        ((currentEpochTime - oldestRecordTime) < MAX_EVENTED_DATA_RETENTION_TIME))
    {
        newRetentionDataTimeInSeconds = currentEpochTime - oldestRecordTime;
        LOGDEBUG("Data retention time is being set to : " << newRetentionDataTimeInSeconds);
    }

    std::string configFile = Common::FileSystem::join(Plugin::osqueryConfigDirectoryPath(), "options.conf");
    Plugin::OsqueryConfigurator::regenerateOsqueryOptionsConfigFile(configFile, newRetentionDataTimeInSeconds);
}

unsigned long OsqueryDataManager::getOldestAllowedTimeForCurrentEventedData()
{
    LOGDEBUG("Checking for time of oldest event, up to " << m_eventsMaxFromConfigAtStart << " events.");
    /**
     * If limit is 100k then the following query will obtain the time stamp of the 100000th record (if it exists) in
     * each evented table and then return the lowest time across the result for each table.
     */
    std::string queryString = buildLimitQueryString(m_eventsMaxFromConfigAtStart);

    auto resultData = runQuery(queryString);

    if (resultData.has_value() && resultData->size() == 1)
    {
        auto row = resultData->back();

        if (row.find("time_to_keep") != row.end())
        {
            if (!row["time_to_keep"].empty())
            {
                LOGINFO("Lowest record time to keep: " << row["time_to_keep"]);
                try
                {
                    return std::stoll(row["time_to_keep"]);
                }
                catch(const std::invalid_argument&)
                {
                    return MAX_EVENTED_DATA_RETENTION_TIME;
                }
                catch (const std::out_of_range&)
                {
                    return MAX_EVENTED_DATA_RETENTION_TIME;
                }
            }
            else
            {
                return MAX_EVENTED_DATA_RETENTION_TIME;
            }
        }
    }
    throw std::runtime_error("Failed to obtain lowest record time to keep time from osquery");
    // return MAX_EVENTED_DATA_RETENTION_TIME;
}

std::optional<OsquerySDK::QueryData> OsqueryDataManager::runQuery(const std::string& query)
{
    if (!m_clientAlive)
    {
        m_clientAlive = connectToOsquery();
    }

    if (m_clientAlive)
    {
        try
        {
            OsquerySDK::QueryData queryData;
            auto status = m_osqueryClient->query(query, queryData);
            LOGDEBUG("Internal query returned status: " << status.code);
            if (status.code == QUERY_SUCCESS)
            {
                return queryData;
            }
            else
            {
                LOGWARN("Failed to run internal data manager query : " << status.code << " " << status.message);
                m_clientAlive = false;
            }
        }
        catch (const std::exception& exception)
        {
            LOGWARN("Failed to run internal data manager query: " << exception.what());
            m_clientAlive = false;
        }
    }
    return std::nullopt;
}

bool OsqueryDataManager::connectToOsquery()
{
    std::optional<std::string> currentMtrSocket = Plugin::osquerySocket();
    if (currentMtrSocket.has_value())
    {
        auto filesystem = Common::FileSystem::fileSystem();
        if (filesystem->exists(currentMtrSocket.value()))
        {
            try
            {
                m_osqueryClient = osqueryclient::factory().create();
                m_osqueryClient->connect(currentMtrSocket.value());
                return true;
            }
            catch (const std::exception& exception)
            {
                LOGERROR("Failed to connect to data manager to OSQuery: " << exception.what());
            }
            catch(...)
            {
                LOGERROR("Failed to connect to OSQuery, unknown error");
            }
        }
    }
    return false;
}

void OsqueryDataManager::purgeDatabase()
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    try
    {
        LOGINFO("Purging osquery database");
        std::string databasePath = Plugin::osQueryDataBasePath();
        if (ifileSystem->isDirectory(databasePath))
        {
            std::vector<std::string> paths = ifileSystem->listFiles(databasePath);

            for (const auto& filepath : paths)
            {
                ifileSystem->removeFile(filepath);
            }

            LOGINFO("Purging Done");
        }
    }
    catch (const Common::FileSystem::IFileSystemException& e)
    {
        LOGERROR("Database cannot be purged due to exception: " << e.what());
    }
}

void OsqueryDataManager::dataCheckThreadMonitorFunc(
    std::shared_ptr<OsqueryDataRetentionCheckState> osqueryDataRetentionCheckState,
    OsqueryDataManager::timepoint_t timeNow)
{
    osqueryDataRetentionCheckState->running = true;
    Common::UtilityImpl::FormattedTime formattedTime;
    unsigned long currentepochTime = 0;
    try
    {
        currentepochTime = std::stoll(formattedTime.currentEpochTimeInSeconds());
    }
    catch (const std::invalid_argument& ex)
    {
        LOGDEBUG("Failed to convert current epoch time, invalid argument error: " << ex.what());
    }
    catch (const std::out_of_range& ex)
    {
        LOGDEBUG("Failed to convert current epoch time, out of range error: " << ex.what());
    }

    while(osqueryDataRetentionCheckState->numberOfRetries > 0 && osqueryDataRetentionCheckState->enabled)
    {
        LOGDEBUG("Number of reconfigure Data Retention tries left: " << osqueryDataRetentionCheckState->numberOfRetries);
        OsqueryDataManager osqueryDataManager;
        try
        {
            LOGDEBUG("Running Reconfigure osquery data retention times");
            osqueryDataManager.reconfigureDataRetentionParameters(
                currentepochTime, osqueryDataManager.getOldestAllowedTimeForCurrentEventedData());
            osqueryDataRetentionCheckState->numberOfRetries = 5;
            LOGDEBUG("Completed Running Reconfigure osquery");
            break;
        }
        catch (const std::runtime_error&)
        {
            osqueryDataManager.reconfigureDataRetentionParameters(currentepochTime, osqueryDataManager.MAX_EVENTED_DATA_RETENTION_TIME);
            osqueryDataRetentionCheckState->numberOfRetries--;

            // Sleep for 10 seconds but also check if we should stop running due to shutdown.
            Common::UtilityImpl::waitFor(10,
                                         0.5,
                                         [&osqueryDataRetentionCheckState](){return !osqueryDataRetentionCheckState->enabled;});
        }
    }
    osqueryDataRetentionCheckState->lastOSQueryDataCheck = timeNow;
    osqueryDataRetentionCheckState->running = false;
}

void OsqueryDataManager::asyncCheckAndReconfigureDataRetention(std::shared_ptr<OsqueryDataRetentionCheckState> osqueryDataRetentionCheckState)
{
    auto timeNow = std::chrono::steady_clock::now();
    if (osqueryDataRetentionCheckState->firstRun ||
    ((osqueryDataRetentionCheckState->numberOfRetries != 5) ||
     (timeNow > (osqueryDataRetentionCheckState->lastOSQueryDataCheck + osqueryDataRetentionCheckState->osqueryDataCheckPeriod))))
    {
        osqueryDataRetentionCheckState->firstRun = false;
        m_dataCheckThreadMonitor = std::async(
            std::launch::async,
            [osqueryDataRetentionCheckState, timeNow]
            {
                dataCheckThreadMonitorFunc(osqueryDataRetentionCheckState, timeNow);
            });
    }
}

std::string OsqueryDataManager::buildLimitQueryString(unsigned int limit)
{
    // Check event max is a sensible value
    if (limit < EVENT_MAX_LOWER_LIMIT || limit > EVENT_MAX_UPPER_LIMIT)
    {
        limit = EVENT_MAX_DEFAULT;
    }

    std::stringstream queryString;
    queryString << "WITH time_values AS (" << std::endl
       << "SELECT (SELECT time FROM process_events ORDER BY time DESC LIMIT 1 OFFSET " << limit << ") AS oldest_time" << std::endl
        << "union" << std::endl
        << "SELECT(SELECT time FROM user_events ORDER BY time DESC LIMIT 1 OFFSET " << limit << ") AS oldest_time" << std::endl
        << "union" << std::endl
        << "SELECT(SELECT time FROM selinux_events ORDER BY time DESC LIMIT 1 OFFSET " << limit << ") AS oldest_time" << std::endl
        << "union" << std::endl
        << "SELECT(SELECT time FROM socket_events ORDER BY time DESC LIMIT 1 OFFSET " << limit << ") AS oldest_time" << std::endl
        << "union" << std::endl
        << "SELECT(SELECT time FROM syslog_events ORDER BY time DESC LIMIT 1 OFFSET " << limit << ") AS oldest_time" << std::endl
        << ")" << std::endl
        << "SELECT MIN(oldest_time) AS time_to_keep FROM time_values WHERE oldest_time > 0";
    return queryString.str();
}

OsqueryDataManager::OsqueryDataManager()
    : m_eventsMaxFromConfigAtStart(Plugin::PluginUtils::getEventsMaxFromConfig())
{
}