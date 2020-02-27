/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelperImpl/TelemetryValue.h>
#include <Common/TelemetryHelperImpl/TelemetryJsonToMap.h>
#include <redist/boost/include/boost/property_tree/ini_parser.hpp>
#include <redist/boost/include/boost/property_tree/ptree.hpp>

namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
    const std::string LOG_MESSAGE = "msg";
    const std::string SPLIT_STRING = "#SPLIT#";
    const std::string OSQUERY_RESTART_CPU_LOG_MSG = "osqueryd worker (" + SPLIT_STRING + ") stopping: Maximum sustainable CPU utilization limit exceeded:";
    const std::string OSQUERY_RESTART_MEMORY_LOG_MSG = "osqueryd worker (" + SPLIT_STRING + ") stopping: Memory limits exceeded:";
    const std::string DATABASE_PURGE_LOG_MSG = "Osquery database watcher purge completed";
    static constexpr unsigned long MAX_SYSLOG_SIZE = 1024 * 1024 * 50; //50Mb limit on file size
}

namespace plugin
{
    std::optional<std::string> getVersion()
    {
        try
        {
            Path versionIniFilepath = Plugin::getVersionIniFilePath();
            boost::property_tree::ptree pTree;
            boost::property_tree::read_ini(versionIniFilepath, pTree);
            return pTree.get<std::string>(PRODUCT_VERSION_STR);
        }
        catch(std::exception& ex)
        {
            LOGERROR("Telemetry cannot find the plugin version");
            return std::nullopt;
        }
    }

//    std::optional<unsigned long> getNumberOfOsqueryRestartsDueToMemory()
//    {
//        // TODO find correct log path to use
//        LogLineCounterAndCache logCounter = LogLineCounterAndCache(Plugin::osQueryWarningLogPath(), OSQUERY_RESTART_MEMORY_LOG_MSG, Plugin::getRestartOSQueryLogLineCacheMemory());
//        return logCounter.countOccurrencesSinceLastCall();
//    }
//
//    std::optional<unsigned long> getNumberOfOsqueryRestartsDueToCPU()
//    {
//        // TODO find correct log path to use
//        LogLineCounterAndCache logCounter = LogLineCounterAndCache(Plugin::osQueryWarningLogPath(), OSQUERY_RESTART_CPU_LOG_MSG, Plugin::getRestartOSQueryLogLineCacheCPU());
//        return logCounter.countOccurrencesSinceLastCall();
//    }
//
//    std::optional<unsigned long> getNumberOfDatabasePurges()
//    {
//        // TODO find correct log path to use
//        JsonLogLineCounterAndCache logCounter = JsonLogLineCounterAndCache(Plugin::osQueryWarningLogPath(), LOG_MESSAGE, DATABASE_PURGE_LOG_MSG, Plugin::getPurgeDatabaseOSQueryLogLineCache());
//        return logCounter.countOccurrencesSinceLastCall();
//    }

//    AbstractLogLineCounterAndCache::AbstractLogLineCounterAndCache(std::string logFilePath, std::string value, std::string cacheFilePath) :
//            m_cacheFilePath(std::move(cacheFilePath)),
//            m_logFilePath(std::move(logFilePath)),
//            m_matchValue(std::move(value))
//    {
//        LoadCachedFileMarker();
//    }

    //  std::optional<unsigned long> AbstractLogLineCounterAndCache::countOccurrencesSinceLastCall()
    //  Method is used to look through log files and return the number of occurrences of CheckLineMatchesTest.
    //  If multiple log files are returned from FilesToCheck they must be in order of the latest modified file
    //  first. The algorithm will then check each file from most recent to least recent;
    //      read each file line by line in reverse order
    //      if line matches the last log line read during previous call it stops checking this file and any older file
    //      if CheckLineMatchesTest returns true increment occurrences
    //      At end of checking all files the lastLogLine is cached for future log checks
    //  The intention is to capture the number of times something has occurred in the logs since the last time this
    //  method was called.
//    std::optional<unsigned long> AbstractLogLineCounterAndCache::countOccurrencesSinceLastCall()
//    {
//        unsigned long occurrences = 0;
//        auto fs = Common::FileSystem::fileSystem();
//        try {
//            std::vector<std::string> filesToCheck = FilesToCheck();
//            std::string lastLogLine;
//            bool exitFileLoop = false;
//            for (auto &files : filesToCheck) {
//                std::vector<std::string> logLines(fs->readLines(files, MAX_SYSLOG_SIZE));
//                for (auto line = logLines.rbegin(); line != logLines.rend(); ++line)
//                {
//                    if (m_lastLogLine && m_lastLogLine == *line)
//                    {
//                        exitFileLoop = true;
//                        break;
//                    }
//                    if (CheckLineMatchesTest(*line))
//                    {
//                        ++occurrences;
//                    }
//                }
//                if (!logLines.empty() && lastLogLine.empty())
//                {
//                    //Iterate back through the file and find the first non-empty log line
//                    for (auto line = logLines.rbegin(); line != logLines.rend(); ++line)
//                    {
//                        if (!line->empty())
//                        {
//                            lastLogLine = *line;
//                            break;
//                        }
//                    }
//                }
//                if (exitFileLoop)
//                {
//                    break;
//                }
//            }
//            if (!lastLogLine.empty())
//            {
//                m_lastLogLine = lastLogLine;
//            }
//            SaveCachedFileMarker();
//            return occurrences;
//        }
//        catch (const std::exception & exception)
//        {
//            LOGWARN("Failed to read log file " << m_logFilePath << " because: " << exception.what());
//            return std::nullopt;
//        }
//    }
//
//    void AbstractLogLineCounterAndCache::LoadCachedFileMarker()
//    {
//        try
//        {
//            std::vector<std::string> logLines = Common::FileSystem::fileSystem()->readLines(m_cacheFilePath);
//            m_lastLogLine = logLines.at(0);
//        }
//        catch (const std::exception & exception)
//        {
//            m_lastLogLine = std::nullopt;
//        }
//    }
//
//    void AbstractLogLineCounterAndCache::SaveCachedFileMarker()
//    {
//        try
//        {
//            if(m_lastLogLine.has_value())
//            {
//                Common::FileSystem::fileSystem()->writeFile(m_cacheFilePath, m_lastLogLine.value());
//            }
//        }
//        catch (const std::exception & exception)
//        {
//            LOGERROR("Failed to cache last log line when generating telemetry from file: " << m_logFilePath);
//        }
//    }
//
//    LogLineCounterAndCache::LogLineCounterAndCache(std::string logFilePath, std::string value, std::string cacheFilePath) :
//            AbstractLogLineCounterAndCache(std::move(logFilePath), std::move(value), std::move(cacheFilePath))
//    {
//        //Split test string into parts if required
//        size_t pos = m_matchValue.find(SPLIT_STRING);
//        size_t start = 0;
//        while (pos != std::string::npos)
//        {
//            m_checkInOrder.emplace_back(m_matchValue.substr(start, pos - start));
//            start = pos + SPLIT_STRING.size();
//            pos = m_matchValue.find(SPLIT_STRING, start);
//        }
//        m_checkInOrder.emplace_back(m_matchValue.substr(start));
//    }
//
//    std::vector<std::string> LogLineCounterAndCache::FilesToCheck()
//    {
//        return {m_logFilePath};
//    }
//
//    bool LogLineCounterAndCache::CheckLineMatchesTest(const std::string & line)
//    {
//        size_t start = 0;
//        for (auto & checkLine : m_checkInOrder)
//        {
//            size_t pos = line.find(checkLine, start);
//            if (pos == std::string::npos)
//            {
//                return false;
//            }
//            start = pos;
//        }
//        return true;
//    }
//
//    JsonLogLineCounterAndCache::JsonLogLineCounterAndCache(std::string logFilePath, std::string key, std::string value, std::string cacheFilePath) :
//            AbstractLogLineCounterAndCache(std::move(logFilePath), std::move(value), std::move(cacheFilePath)),
//            m_searchKey(std::move(key))
//    {}
//
//    std::vector<std::string> JsonLogLineCounterAndCache::FilesToCheck()
//    {
//        return {m_logFilePath};
//    }
//
//    bool JsonLogLineCounterAndCache::CheckLineMatchesTest(const std::string & line)
//    {
//        try
//        {
//            std::unordered_map<std::string, Common::Telemetry::TelemetryValue> logLineJsonMap = Common::Telemetry::flatJsonToMap(
//                    line);
//            auto foundIter = logLineJsonMap.find(m_searchKey);
//            return foundIter != logLineJsonMap.end() && foundIter->second.getString() == m_matchValue;
//        }
//        catch (const std::runtime_error & exception)
//        {
//            //Failure to read one line of json in a file shouldn't stop the whole read
//            return false;
//        }
//    }

} // namespace plugin

