// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include <Common/PersistentValue/PersistentValue.h>

#include <common/LockableData.h>

#include <chrono>
#include <list>
#include <map>

namespace Plugin
{
    namespace JsonKeys {
        constexpr auto threatId = "threatId";
        constexpr auto correlationId = "correlationId";
        constexpr auto timestamp = "timestamp";
    }

    class ThreatDatabase
    {
        public:
            explicit ThreatDatabase(const std::string& path);
            ~ThreatDatabase();
            void addThreat(const std::string& threatID, const std::string& correlationID);
            void removeCorrelationID(const std::string& correlationID);
            void removeThreatID(const std::string& threatID, bool ignoreNotInDatabase=true);
            void resetDatabase();
            bool isDatabaseEmpty() const;
            bool isThreatInDatabaseWithinTime(const std::string& threatId, const std::chrono::seconds& duplicateTimeout) const;

        TEST_PUBLIC:
            struct ThreatDetails
            {
                std::string correlationId;
                //This is saved to disk when ThreatDatabase is destroyed, so use system clock
                std::chrono::system_clock::time_point lastDetection = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());

                ThreatDetails(const std::string& _correlationId)
                {
                    correlationId = _correlationId;
                }

                ThreatDetails(const std::string& _correlationId, const long& _time)
                {
                    correlationId = _correlationId;
                    lastDetection = std::chrono::system_clock::time_point(std::chrono::seconds(_time));
                }
            };

            void convertDatabaseToString();
            void convertStringToDatabase();
            mutable common::LockableData<std::map<std::string, ThreatDetails>> m_database;
            Common::PersistentValue<std::string> m_databaseInString;
    };
}

