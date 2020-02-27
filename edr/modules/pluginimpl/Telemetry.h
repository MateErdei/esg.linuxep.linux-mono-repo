/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/FileSystem/IFileSystem.h>

#include <optional>
#include <string>

namespace plugin
{
    std::optional<std::string> getVersion();
//    std::optional<unsigned long> getNumberOfDatabasePurges();
//    std::optional<unsigned long> getNumberOfOsqueryRestartsDueToMemory();
//    std::optional<unsigned long> getNumberOfOsqueryRestartsDueToCPU();

//    class AbstractLogLineCounterAndCache
//    {
//    public:
//        AbstractLogLineCounterAndCache(std::string logFilePath, std::string stringToMatch, std::string cacheFilePath);
//        virtual ~AbstractLogLineCounterAndCache() = default;
//        /*
//         * Count the number of occurrences CheckLineMatchesTest returning true in the given log file
//         */
//        std::optional<unsigned long> countOccurrencesSinceLastCall();
//    private:
//        /*
//         * Performs the check on a line to check the match condition
//         */
//        virtual bool CheckLineMatchesTest(const std::string & line) = 0;
//        /*
//         * Returns a list of files to check log contents of
//         */
//        virtual std::vector<std::string> FilesToCheck() = 0;
//        /*
//         * Loads the last checked log line from cache file on disk
//        */
//        void LoadCachedFileMarker();
//        /*
//         * Save the last checked log line to cache file on disk
//         */
//        void SaveCachedFileMarker();
//
//        std::string m_cacheFilePath;
//        std::optional<std::string> m_lastLogLine;
//    protected:
//        std::string m_logFilePath;
//        std::string m_matchValue;
//    };
//
//    class LogLineCounterAndCache : public AbstractLogLineCounterAndCache
//    {
//    public:
//        LogLineCounterAndCache(std::string logFilePath, std::string stringToMatch, std::string cacheFilePath);
//    private:
//        bool CheckLineMatchesTest(const std::string & line) override;
//        std::vector<std::string> FilesToCheck() override;
//    public:
//        std::vector<std::string> m_checkInOrder;
//    };
//
//    class JsonLogLineCounterAndCache : public AbstractLogLineCounterAndCache
//    {
//    public:
//        JsonLogLineCounterAndCache(std::string logFilePath, std::string key, std::string value, std::string cacheFilePath);
//    private:
//        bool CheckLineMatchesTest(const std::string & line) override;
//        std::vector<std::string> FilesToCheck() override;
//        std::string m_searchKey;
//    };



} // namespace Plugin