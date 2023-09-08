// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ResultsSender.h"

#include "Logger.h"
#include "EdrConstants.h"
#include <Common/FileSystem/IFileSystemException.h>

#include <Common/UtilityImpl/TimeUtils.h>
#include <json/json.h>
#include <modules/pluginimpl/ApplicationPaths.h>


#include <iostream>
#include <utility>

ResultsSender::ResultsSender(
    std::string intermediaryPath,
    std::string datafeedPath,
    std::string osqueryXDRConfigFilePath,
    std::string osqueryMTRConfigFilePath,
    std::string osqueryCustomConfigFilePath,
    const std::string& pluginVarDir,
    unsigned long long int dataLimit,
    unsigned int periodInSeconds,
    std::function<void(void)> dataExceededCallback) :
    m_intermediaryPath(std::move(intermediaryPath)),
    m_datafeedPath(std::move(datafeedPath)),
    m_osqueryXDRConfigFilePath(std::move(osqueryXDRConfigFilePath)),
    m_osqueryMTRConfigFilePath(std::move(osqueryMTRConfigFilePath)),
    m_osqueryCustomConfigFilePath(std::move(osqueryCustomConfigFilePath)),
    m_currentDataUsage(pluginVarDir, "xdrDataUsage", 0),
    m_periodStartTimestamp(
        pluginVarDir,
        "xdrPeriodTimestamp",
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()),
    m_dataLimit(dataLimit),
    m_periodInSeconds(pluginVarDir, "xdrPeriodInSeconds", periodInSeconds),
    m_dataExceededCallback(std::move(dataExceededCallback)),
    m_hitLimitThisPeriod(pluginVarDir, "xdrLimitHit", false)
{
    LOGDEBUG("Created results sender");
    try
    {
        loadScheduledQueryTags();
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to load scheduled query tags on start up. " << e.what());
    }
    LOGDEBUG("Initial XDR data limit: " << m_dataLimit << " bytes");
    LOGDEBUG("Initial XDR data limit roll over period: " << m_periodInSeconds.getValue() << " seconds");
    LOGDEBUG("Initial XDR data limit roll over timestamp: " << m_periodStartTimestamp.getValue());
}

std::optional<std::string> ResultsSender::PrepareSingleResult(const std::string& result)
{
    LOGDEBUG("Adding XDR results to intermediary file: " << result);

    Json::Value logLine;
    std::string queryName;
    try
    {
        std::stringstream logLineStream(result);
        logLineStream >> logLine;
        queryName = logLine["name"].asString();
        if (result.size() > EdrCommon::DEFAULT_MAX_BATCH_SIZE_BYTES)
        {
            LOGWARN("Query row in " << queryName << " result bigger than 2MB. Query row dropped.");
            return std::nullopt;
        }
        if (!queryName.empty())
        {
            const auto& [name, correctedTag] = m_scheduledQueryTagMap[queryName];
            if (logLine["name"] != name && !name.empty())
            {
                logLine["name"] = name;
            }
            if (correctedTag.empty())
            {
                LOGDEBUG("Query "<< queryName << " is untagged we will discard the data for this query");
                return std::nullopt;
            }
            logLine["tag"] = correctedTag;
        }
    }
    catch (const std::exception& e)
    {
        LOGERROR("Invalid JSON log message. " << e.what());
        return std::nullopt;
    }
    if (m_mtrLicense)
    {
        if (logLine.isMember("decorations"))
        {
            logLine["decorations"]["licence"] = "MTR";
        }
    }
    std::stringstream ss;
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";
    ss << Json::writeString(writerBuilder, logLine);

    m_currentDataUsage.setValue(m_currentDataUsage.getValue() + ss.str().length());

    // Record that this data has caused us to go over limit.
    if (m_currentDataUsage.getValue() > m_dataLimit && !m_hitLimitThisPeriod.getValue())
    {
        LOGWARN("XDR data limit for this period exceeded");
        m_hitLimitThisPeriod.setValue(true);
        try
        {
            m_dataExceededCallback();
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Data exceeded callback threw an exception: " << ex.what());
        }
    }

    return ss.str();
}

void ResultsSender::Add(const std::string& result)
{
    if (result.empty())
    {
        return;
    }

    std::string stringToAppend;
    if (!m_firstEntry)
    {
        stringToAppend = ",";
    }

    stringToAppend.append(result);
    auto filesystem = Common::FileSystem::fileSystem();
    filesystem->appendFile(m_intermediaryPath, stringToAppend);
    m_firstEntry = false;
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
    LOGDEBUG("ResultsSender::Reset");
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
    std::string jsonFileContent;
    try
    {
        auto filesystem = Common::FileSystem::fileSystem();
        jsonFileContent = filesystem->readFile(path);
        reader->parse(jsonFileContent.c_str(), jsonFileContent.c_str() + jsonFileContent.size(), &root, &errors);
    }
    catch (const std::exception& err)
    {
        throw std::runtime_error("Failed to open " + path + " to read. Error:" + err.what());
    }

    if (!errors.empty())
    {
        LOGDEBUG("JSON content that failed to be parsed: " << jsonFileContent);
        throw std::runtime_error("Failed to parse " + path + " Error:" + errors);
    }

    return root;
}

void ResultsSender::loadScheduledQueryTagsFromFile(std::vector<ScheduledQuery> &scheduledQueries, Path queryPackFilePath)
{
    LOGSUPPORT("Attempting to read " << queryPackFilePath << " for query tags");
    auto fs = Common::FileSystem::fileSystem();
    Json::Value confJsonRoot;
    std::string disabledQueryPackPath = queryPackFilePath + ".DISABLED";
    if (fs->exists(queryPackFilePath))
    {
        confJsonRoot = readJsonFile(queryPackFilePath);
        LOGDEBUG("Reading " << queryPackFilePath << " for query tags");
    }
    else if (fs->exists(disabledQueryPackPath))
    {
        confJsonRoot = readJsonFile(disabledQueryPackPath);
        LOGDEBUG("Reading " << disabledQueryPackPath << " for query tags");
    }
    else
    {
        LOGERROR("Failed to find query pack to extract scheduled query tags from: " << queryPackFilePath);
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
    auto otherQueryPacks = confJsonRoot["packs"];
    for (Json::Value::const_iterator packItr = otherQueryPacks.begin(); packItr != otherQueryPacks.end(); packItr++)
    {
        auto packName = packItr.key().asString();
        auto packNode = *packItr;
        auto packQueries = packNode["queries"];
        for (Json::Value::const_iterator packQueriesItr = packQueries.begin(); packQueriesItr != packQueries.end();
             packQueriesItr++)
        {
            auto query = *packQueriesItr;
            std::string packAppendedName = "pack_" + packName + "_" + packQueriesItr.key().asString();
            scheduledQueries.push_back(
                    ScheduledQuery { packAppendedName, packQueriesItr.key().asString(), query["tag"].asString() });
        }
    }
}

void ResultsSender::loadScheduledQueryTags()
{
    std::vector<ScheduledQuery> scheduledQueries;
    loadScheduledQueryTagsFromFile(scheduledQueries, m_osqueryXDRConfigFilePath);
    loadScheduledQueryTagsFromFile(scheduledQueries, m_osqueryMTRConfigFilePath);
    auto fs = Common::FileSystem::fileSystem();
    if (fs->exists(m_osqueryCustomConfigFilePath))
    {
        loadScheduledQueryTagsFromFile(scheduledQueries, m_osqueryCustomConfigFilePath);
    }

    std::map<std::string, std::pair<std::string, std::string>> queryTagMap;
    for (const auto& query : scheduledQueries)
    {
        if (queryTagMap.find(query.queryNameWithPack) != queryTagMap.end())
        {
            LOGINFO(query.queryNameWithPack << " already in query map");
        }
        queryTagMap.insert(std::make_pair(query.queryNameWithPack, std::make_pair(query.queryName, query.tag)));
    }
    m_scheduledQueryTagMap = queryTagMap;
}

void ResultsSender::setDataLimit(unsigned long long int limitbytes)
{
    m_dataLimit = limitbytes;
}

void ResultsSender::setMtrLicense(bool hasMTRLicense)
{
    m_mtrLicense = hasMTRLicense;
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

unsigned long long int ResultsSender::getDataLimit()
{
    return m_dataLimit;
}

void ResultsSender::SaveBatchResults(const Json::Value& results)
{
    auto filesystem = Common::FileSystem::fileSystem();
    if (results.empty())
    {
        return;
    }


    try
    {
        LOGDEBUG("Saving batch results");
        Json::StreamWriterBuilder builder;
        std::stringstream resultsStringStream;

        // Make sure there are no indentations or new lines by setting empty string
        builder["indentation"] = "";

        // Make sure that "null" is not written out if the results object is empty
        builder["dropNullPlaceholders"] = true;

        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        filesystem->writeFile(m_intermediaryPath, Json::writeString(builder, results));
    }
    catch (const std::exception& exception)
    {
        LOGERROR(  "Failed to save result batch file " << m_intermediaryPath << " Error: " << exception.what());
    }
}

Json::Value ResultsSender::PrepareBatchResults()
{
    Json::Value batchResults;
    auto filesystem = Common::FileSystem::fileSystem();
    if (filesystem->exists(m_intermediaryPath))
    {
        bool batchFileIsValidJson = false;
        try
        {
            filesystem->appendFile(m_intermediaryPath, "]");
            batchResults = readJsonFile(m_intermediaryPath);
            batchFileIsValidJson = true;
        }
        catch (const std::exception& exception)
        {
            LOGERROR(
                "Failed to prepare result batch file " << m_intermediaryPath << " Error: " << exception.what()
                                                     << ". Deleting File");
        }

        if (!batchFileIsValidJson)
        {
            try
            {
                filesystem->removeFile(m_intermediaryPath);
            }
            catch (Common::FileSystem::IFileSystemException& fileSystemException)
            {
                LOGERROR(
                    "Failed to remove invalid result batch file " << m_intermediaryPath
                                                                  << " Error: " << fileSystemException.what());
            }
        }
    }
    return batchResults;
}
