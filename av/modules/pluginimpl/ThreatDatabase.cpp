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
            auto correlationItr = std::find(dbItr->second.correlationIds.begin(), dbItr->second.correlationIds.end(), correlationID);
            if (correlationItr == dbItr->second.correlationIds.end() )
            {
                dbItr->second.correlationIds.emplace_back(correlationID);
            }
            dbItr->second.lastDetection = std::chrono::system_clock::now();
        }
        else
        {
            auto newThreatDetails = ThreatDetails(correlationID);
            database->emplace(threatID,std::move(newThreatDetails));
        }

    }

    void ThreatDatabase::removeCorrelationID(const std::string& correlationID)
    {
        auto database = m_database.lock();
        std::string threatID = "";
        for (auto const& entry : *database)
        {
            auto correlationItr = std::find(entry.second.correlationIds.begin(), entry.second.correlationIds.end(), correlationID);
            if (correlationItr != entry.second.correlationIds.end() )
            {
                threatID = entry.first;
            }
        }

        if (threatID.empty())
        {
            LOGINFO("Cannot remove correlation id" << correlationID << " from database as it cannot be found");
        }
        else
        {
            auto threatItr = database->find(threatID);
            database->erase(threatItr);
            LOGDEBUG("Removed threat from database");
        }
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
            LOGWARN("Cannot remove threat id" << threatID << " from database as it cannot be found");
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
        auto threatItr = database->find(threatId);
        if (threatItr == database->cend())
        {
            return false;
        }

        std::chrono::system_clock::time_point timePointNow = std::chrono::system_clock::now();
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
            j[key.first] = { {"correlationIds", key.second.correlationIds},
                             {"timestamp", timeStamp }};
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
            //this one is a warn as we can recover from this
            LOGWARN("Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error: " << ex.what());
            if (Common::FileSystem::fileSystem()->exists(Plugin::getPersistThreatDatabaseFilePath()))
            {
                Common::Telemetry::TelemetryHelper::getInstance().set("corrupt-threat-database", true);
            }
        }

        std::map<std::string, ThreatDetails> tempdatabase;

        for (const auto& threatItr : j.items())
        {
            std::list<std::string> correlationIdsToPop;
            try
            {
                auto correlationIds = threatItr.value().at("correlationIds").items();
                for (auto corrItr : correlationIds)
                {
                    correlationIdsToPop.emplace_back(corrItr.value());
                }
            }
            catch (nlohmann::json::exception& ex)
            {
                //If the types of the threat id or correlation id are wrong throw away the entire threatID entry
                LOGWARN("Not loading "<< threatItr.key() << " into threat database as the parsing failed with error " << ex.what());
                continue;
            }

            long timeStamp = 0;
            try
            {
                timeStamp = threatItr.value().at("timestamp");
            }
            catch (nlohmann::json::exception& ex)
            {
                LOGWARN("Time field for " << threatItr.key() << " is invalid, setting to 0: " << ex.what());
                timeStamp = 0;
            }

            ThreatDetails details(correlationIdsToPop, timeStamp);
            tempdatabase.try_emplace(threatItr.key(), details);
        }

        database->swap(tempdatabase);
        LOGINFO("Initialised Threat Database");
    }
}