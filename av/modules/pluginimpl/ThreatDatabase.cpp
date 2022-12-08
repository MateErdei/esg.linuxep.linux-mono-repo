// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ThreatDatabase.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <common/ApplicationPaths.h>

#include <thirdparty/nlohmann-json/json.hpp>
#include <algorithm>
#include "Logger.h"
namespace Plugin
{
    ThreatDatabase::ThreatDatabase(const std::string& path):
        m_databaseInString(path,"threatDatabase","{}")
    {
        convertStringToDatabase();
    }

    ThreatDatabase::~ThreatDatabase()
    {
        convertDatabaseToString();
    }

    void ThreatDatabase::addThreat(const std::string& threatID, const std::string& correlationID)
    {
        auto database = m_database.lock();
        auto dbItr = database->find(threatID);
        if (dbItr != database->end())
        {
            auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
            long duration = (now - dbItr->second.lastDetection).count();
            LOGDEBUG("ThreatID: " << threatID << " already existed. Overwriting correlationID: "
                     << dbItr->second.correlationId << ", age: " << duration << " with " << correlationID);

            dbItr->second.correlationId = correlationID;
            dbItr->second.lastDetection = now;
        }
        else
        {
            auto newThreatDetails = ThreatDetails(correlationID);
            database->emplace(threatID,std::move(newThreatDetails));
            LOGDEBUG("Added threat " << threatID << " with correlationID " << correlationID << " to threat database");
        }

    }

    void ThreatDatabase::removeCorrelationID(const std::string& correlationID)
    {
        auto database = m_database.lock();

        for (auto threatItr = database->begin(); threatItr != database->end();)
        {
            if (threatItr->second.correlationId == correlationID)
            {
                LOGDEBUG("Removing threat " << threatItr->first << " with correlationId " << correlationID << " from database");
                database->erase(threatItr);
                return;
            }
            else
            {
                ++threatItr;
            }
        }

        LOGINFO("Cannot remove correlation id: " << correlationID << " from database as it cannot be found");
    }

    void ThreatDatabase::removeThreatID(const std::string& threatID, bool ignoreNotInDatabase)
    {
        auto database = m_database.lock();

        auto threatItr = database->find(threatID);
        if (threatItr != database->end())
        {
            database->erase(threatItr);
            LOGDEBUG("Removed threat id " << threatID << " from database");
        }
        else if (!ignoreNotInDatabase)
        {
            LOGWARN("Cannot remove threat id " << threatID << " from database as it cannot be found");
        }
    }

    bool ThreatDatabase::isDatabaseEmpty() const
    {
        auto database = m_database.lock();
        return database->empty();
    }

    bool ThreatDatabase::isThreatInDatabaseWithinTime(const std::string& threatId, const std::chrono::seconds& duplicateTimeout) const
    {
        auto database = m_database.lock();

        if (database->empty())
        {
            return false;
        }

        auto threatItr = database->find(threatId);
        if (threatItr == database->cend())
        {
            return false;
        }

        auto timePointNow = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());;
        if ((timePointNow - threatItr->second.lastDetection) <= duplicateTimeout)
        {
            return true;
        }
        return false;
    }

    void ThreatDatabase::resetDatabase()
    {
        auto database = m_database.lock();
        database->clear();
        try
        {
            m_databaseInString.setValueAndForceStore("{}");
        }
        catch (Common::FileSystem::IFileSystemException &ex)
        {
            LOGERROR("Cannot reset ThreatDatabase on disk with error: " << ex.what());
        }
    }

    void ThreatDatabase::convertDatabaseToString()
    {
        auto database = m_database.lock();
        nlohmann::json j;

        for (const auto& key : *database)
        {
            long timeStamp = std::chrono::time_point_cast<std::chrono::seconds>(key.second.lastDetection).time_since_epoch().count();
            j[key.first] = { {JsonKeys::correlationId, key.second.correlationId},
                             {JsonKeys::timestamp, timeStamp }};
        }
        if (j.empty())
        {
            m_databaseInString.setValue("{}");
        }
        else
        {
            m_databaseInString.setValue(j.dump());
        }
    }

    void ThreatDatabase::convertStringToDatabase()
    {
        auto database = m_database.lock();
        nlohmann::json j;

        try
        {
            j = nlohmann::json::parse(m_databaseInString.getValue());
        }
        catch (nlohmann::json::exception& ex)
        {
            //Error as we end up with a empty database
            LOGERROR("Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error: " << ex.what());
            if (Common::FileSystem::fileSystem()->exists(Plugin::getPersistThreatDatabaseFilePath()))
            {
                Common::Telemetry::TelemetryHelper::getInstance().set("corrupt-threat-database", true);
            }
        }

        std::map<std::string, ThreatDetails> tempdatabase;

        for (const auto& threatItr : j.items())
        {
            std::string correlationId = "";
            try
            {
                correlationId = threatItr.value().at(JsonKeys::correlationId);
            }
            catch (nlohmann::json::exception& ex)
            {
                LOGWARN("Correlation field for "<< threatItr.key() << " into threat database as parsing failed with error for " << JsonKeys::correlationId  << " " << ex.what());
                Common::Telemetry::TelemetryHelper::getInstance().set("corrupt-threat-database", true);
            }

            long timeStamp = 0;
            try
            {
                timeStamp = threatItr.value().at(JsonKeys::timestamp);
            }
            catch (nlohmann::json::exception& ex)
            {
                LOGWARN("Time field for " << threatItr.key() << " into threat database as parsing failed with error for " << JsonKeys::timestamp << " " << ex.what());
                Common::Telemetry::TelemetryHelper::getInstance().set("corrupt-threat-database", true);
            }

            ThreatDetails details(correlationId, timeStamp);
            tempdatabase.try_emplace(threatItr.key(), details);
        }

        database->swap(tempdatabase);
        LOGINFO("Initialised Threat Database");
    }
}