/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ResultsSender.h"

#include "Logger.h"

#include <modules/pluginimpl/ApplicationPaths.h>
#include <modules/pluginimpl/TelemetryConsts.h>

#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <json/json.h>
#include <iostream>

ResultsSender::ResultsSender(
    const std::string& intermediaryPath,
    const std::string& datafeedPath,
    const std::string& osqueryXDRConfigFilePath,
    const std::string& osqueryMTRConfigFilePath,
    const std::string& pluginVarDir,
    unsigned int dataLimit,
    unsigned int periodInSeconds,
    std::function<void(void)> dataExceededCallback) :
    m_intermediaryPath(intermediaryPath),
    m_datafeedPath(datafeedPath),
    m_osqueryXDRConfigFilePath(osqueryXDRConfigFilePath),
    m_osqueryMTRConfigFilePath(osqueryMTRConfigFilePath),
    m_currentDataUsage(pluginVarDir, "xdrDataUsage", 0),
    m_periodStartTimestamp(
        pluginVarDir,
        "xdrPeriodTimestamp",
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()),
    m_dataLimit(dataLimit),
    m_periodInSeconds(pluginVarDir, "xdrPeriodInSeconds", periodInSeconds),
    m_dataExceededCallback(dataExceededCallback),
    m_hitLimitThisPeriod(pluginVarDir, "xdrLimitHit", false)
{
    LOGDEBUG("Created results sender");
    try
    {
        // cppcheck-suppress virtualCallInConstructor
        Send();
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to send previous batched scheduled results on start up. " << e.what());
    }

    try
    {
        loadScheduledQueryTags();
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to load scheduled query tags on start up. " << e.what());
    }
}

ResultsSender::~ResultsSender()
{
    try
    {
        // cppcheck-suppress virtualCallInConstructor
        Send();
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to send batched scheduled results on shutdown. " << e.what());
    }
}

void ResultsSender::Add(const std::string& result)
{
    LOGDEBUG("Adding XDR results to intermediary file: " << result);
    auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();


    // Check if it has been longer than the data limit period, if it has then reset the data counter.
    checkDataPeriodElapsed();

    // The total data usage if we were to add this result
    unsigned int incrementedDataUsage = m_currentDataUsage.getValue() + result.length();

    // Record that this data has caused us to go over limit.
    if (incrementedDataUsage > m_dataLimit)
    {
        LOGWARN("XDR data limit for this period exceeded");
        m_hitLimitThisPeriod.setValue(true);
        m_dataExceededCallback();
    }

    Json::Value logLine;
    std::string queryName;
    try
    {
        std::stringstream logLineStream(result);
        logLineStream >> logLine;
        std::map<std::string, std::pair<std::string, std::string>> queryTagMap = getQueryTagMap();
        queryName = logLine["name"].asString();
        if (!queryName.empty())
        {
            auto correctQueryNameAndTag = queryTagMap[queryName];
            if (logLine["name"] != correctQueryNameAndTag.first && !correctQueryNameAndTag.first.empty())
            {
                logLine["name"] = correctQueryNameAndTag.first;
            }
            if (!correctQueryNameAndTag.second.empty())
            {
                logLine["tag"] = correctQueryNameAndTag.second;
            }
        }
    }
    catch (const std::exception& e)
    {
        LOGERROR("Invalid JSON log message. " << e.what());
        return;
    }

    std::stringstream key;
    key << plugin::telemetryScheduledQueries << "." << queryName;
    std::string scheduledQueryKey = key.str();

    telemetryHelper.appendStat(scheduledQueryKey + "." + plugin::telemetryRecordSize, result.length());
    telemetryHelper.increment(scheduledQueryKey + "." + plugin::telemetryRecordsCount, 1L);

    std::stringstream ss;
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";
    ss << Json::writeString(writerBuilder, logLine);
    auto taggedResult = ss.str();

    std::string stringToAppend;
    if (!m_firstEntry)
    {
        stringToAppend = ",";
    }

    stringToAppend.append(taggedResult);
    auto filesystem = Common::FileSystem::fileSystem();
    filesystem->appendFile(m_intermediaryPath, stringToAppend);
    m_firstEntry = false;

    m_currentDataUsage.setValue(incrementedDataUsage);
}

void ResultsSender::Send()
{
    if (m_hitLimitThisPeriod.getValue())
    {
        return;
    }
    auto filesystem = Common::FileSystem::fileSystem();
    if (filesystem->exists(m_intermediaryPath))
    {
        filesystem->appendFile(m_intermediaryPath, "]");
        auto filePermissions = Common::FileSystem::filePermissions();
        filePermissions->chown(m_intermediaryPath, "sophos-spl-local", "sophos-spl-group");
        filePermissions->chmod(m_intermediaryPath, 0640);
        Common::UtilityImpl::FormattedTime time;
        std::string now = time.currentEpochTimeInSeconds();
        std::stringstream fileName;
        fileName << std::string("scheduled_query-") << now << ".json";
        Path outputFilePath = Common::FileSystem::join(m_datafeedPath, fileName.str());

        LOGDEBUG("Sending batched XDR scheduled query results - " << outputFilePath);

        filesystem->moveFile(m_intermediaryPath, outputFilePath);
    }
}

void ResultsSender::Reset()
{
    LOGDEBUG("Reset");
    auto filesystem = Common::FileSystem::fileSystem();
    if (filesystem->exists(m_intermediaryPath))
    {
        filesystem->removeFile(m_intermediaryPath);
    }
    filesystem->appendFile(m_intermediaryPath, "[");
    m_firstEntry = true;
}

uintmax_t ResultsSender::GetFileSize()
{
    uintmax_t size = 0;
    auto filesystem = Common::FileSystem::fileSystem();
    if (filesystem->exists(m_intermediaryPath))
    {
        // Add 2 here for the , and ] that gets added
        size = filesystem->fileSize(m_intermediaryPath) + 2;
    }
    return size;
}

Json::Value ResultsSender::readJsonFile(const std::string& path)
{
    Json::CharReaderBuilder builder;
    auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
    Json::Value root;
    std::string errors;
    try
    {
        auto filesystem = Common::FileSystem::fileSystem();
        std::string jsonFile = filesystem->readFile(path);
        reader->parse(jsonFile.c_str(), jsonFile.c_str() + jsonFile.size(), &root, &errors);
    }
    catch (const std::exception& err)
    {
        throw std::runtime_error("Failed to open " + path + " to read. Error:" + err.what());
    }

    return root;
}

void ResultsSender::loadScheduledQueryTagsFromFile(std::vector<ScheduledQuery> &scheduledQueries, Path queryPackFilePath)
{
    auto fs = Common::FileSystem::fileSystem();
    Json::Value confJsonRoot;
    std::string disabledQueryPackPath = queryPackFilePath + ".DISABLED";
    if (fs->exists(m_osqueryXDRConfigFilePath))
    {
        confJsonRoot = readJsonFile(m_osqueryXDRConfigFilePath);
    }
    else if (fs->exists(disabledQueryPackPath))
    {
        confJsonRoot = readJsonFile(disabledQueryPackPath);
    }
    else
    {
        LOGERROR("Failed to find query pack to extract scheduled query tags from");
        return;
    }

    auto scheduleRoot = confJsonRoot["schedule"];
    for (Json::Value::const_iterator scheduledItr = scheduleRoot.begin(); scheduledItr != scheduleRoot.end();
         scheduledItr++)
    {
        auto query = *scheduledItr;
        scheduledQueries.push_back(
                ScheduledQuery { scheduledItr.key().asString(), scheduledItr.key().asString(), query["tag"].asString() });
    }
}

void ResultsSender::loadScheduledQueryTags()
{
    std::vector<ScheduledQuery> scheduledQueries;
    loadScheduledQueryTagsFromFile(scheduledQueries, m_osqueryXDRConfigFilePath);

    m_scheduledQueryTags = scheduledQueries;
}
std::map<std::string, std::pair<std::string, std::string>> ResultsSender::getQueryTagMap()
{
    std::map<std::string, std::pair<std::string, std::string>> queryTagMap;
    for (const auto& query : m_scheduledQueryTags)
    {
        queryTagMap.insert(std::make_pair(query.queryNameWithPack, std::make_pair(query.queryName, query.tag)));
    }
    return queryTagMap;
}

void ResultsSender::setDataLimit(unsigned int limitbytes)
{
    m_dataLimit = limitbytes;
}

void ResultsSender::setDataPeriod(unsigned int periodSeconds)
{
    m_periodInSeconds.setValue(periodSeconds);
}

bool ResultsSender::checkDataPeriodElapsed()
{
    unsigned int now =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (now - m_periodStartTimestamp.getValue() > m_periodInSeconds.getValue())
    {
        m_currentDataUsage.setValue(0);
        m_periodStartTimestamp.setValue(now);
        m_hitLimitThisPeriod.setValue(false);
        LOGDEBUG("XDR period has rolled over");
        return true;
    }
    return false;
}

bool ResultsSender::getDataLimitReached()
{
    return m_hitLimitThisPeriod.getValue();
}
