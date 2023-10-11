// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "ThreatDatabase.h"

#include "Logger.h"

#include "common/ApplicationPaths.h"

#include "datatypes/IUuidGenerator.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <thirdparty/nlohmann-json/json.hpp>

#include <algorithm>

namespace Plugin
{
    ThreatDatabase::ThreatDatabase(const std::string& path) : m_databaseInString(path, "threatDatabase", "{}")
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
            long duration = std::chrono::duration_cast<std::chrono::seconds>(now - dbItr->second.lastDetection).count();
            LOGDEBUG(
                "ThreatId: " << threatID << " already existed. Overwriting correlationId: "
                             << dbItr->second.correlationId << ", age: " << duration << " with " << correlationID);

            dbItr->second.correlationId = correlationID;
            dbItr->second.lastDetection = now;
        }
        else
        {
            auto newThreatDetails = ThreatDetails(correlationID);
            database->emplace(threatID, std::move(newThreatDetails));
            LOGDEBUG("Added threat " << threatID << " with correlationId " << correlationID << " to threat database");
        }
    }

    void ThreatDatabase::removeCorrelationID(const std::string& correlationID)
    {
        auto database = m_database.lock();

        for (auto threatItr = database->begin(); threatItr != database->end(); ++threatItr)
        {
            if (threatItr->second.correlationId == correlationID)
            {
                LOGDEBUG(
                    "Removing threat " << threatItr->first << " with correlationId " << correlationID
                                       << " from database");
                database->erase(threatItr);
                return;
            }
        }

        LOGINFO("Cannot remove correlationId: " << correlationID << " from database as it cannot be found");
    }

    void ThreatDatabase::removeThreatID(const std::string& threatID, bool ignoreNotInDatabase)
    {
        auto database = m_database.lock();

        auto threatItr = database->find(threatID);
        if (threatItr != database->end())
        {
            database->erase(threatItr);
            LOGDEBUG("Removed threatId " << threatID << " from database");
        }
        else if (!ignoreNotInDatabase)
        {
            LOGWARN("Cannot remove threatId " << threatID << " from database as it cannot be found");
        }
    }

    bool ThreatDatabase::isDatabaseEmpty() const
    {
        auto database = m_database.lock();
        return database->empty();
    }

    bool ThreatDatabase::isThreatInDatabaseWithinTime(
        const std::string& threatId,
        const std::chrono::seconds& duplicateTimeout) const
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

        auto timePointNow = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
        if ((timePointNow - threatItr->second.lastDetection) > duplicateTimeout)
        {
            return false;
        }
        return true;
    }

    void ThreatDatabase::resetDatabase()
    {
        auto database = m_database.lock();
        database->clear();
        try
        {
            m_databaseInString.setValueAndForceStore("{}");
            LOGDEBUG("Threat Database has been reset");
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Cannot reset ThreatDatabase on disk with error: " << ex.what());
        }
    }

    std::optional<std::string> ThreatDatabase::hasThreat(const std::string& threatId) const
    {
        auto database = m_database.lock();
        for (auto const& entry : *database)
        {
            if (entry.first == threatId)
            {
                return entry.second.correlationId;
            }
        }

        return {};
    }

    void ThreatDatabase::convertDatabaseToString()
    {
        auto database = m_database.lock();
        nlohmann::json j;

        for (const auto& key : *database)
        {
            long timeStamp =
                std::chrono::time_point_cast<std::chrono::seconds>(key.second.lastDetection).time_since_epoch().count();
            j[key.first] = { { JsonKeys::correlationId, key.second.correlationId },
                             { JsonKeys::timestamp, timeStamp } };
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
            // Error as we end up with a empty database
            LOGERROR("Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error: " << ex.what());
            if (Common::FileSystem::fileSystem()->exists(Plugin::getPersistThreatDatabaseFilePath()))
            {
                Common::Telemetry::TelemetryHelper::getInstance().set("corrupt-threat-database", true);
            }
        }

        std::map<std::string, ThreatDetails> tempdatabase;

        for (const auto& threatItr : j.items())
        {
            std::string correlationId = datatypes::uuidGenerator().generate();
            try
            {
                correlationId = threatItr.value().at(JsonKeys::correlationId);
            }
            catch (nlohmann::json::exception& ex)
            {
                LOGWARN(
                    "Failed to load saved correlation field for " << threatItr.key()
                                                         << " as parsing failed with error for "
                                                         << JsonKeys::correlationId << " " << ex.what());
                Common::Telemetry::TelemetryHelper::getInstance().set("corrupt-threat-database", true);
            }

            long timeStamp = 0;
            try
            {
                timeStamp = threatItr.value().at(JsonKeys::timestamp);
            }
            catch (nlohmann::json::exception& ex)
            {
                LOGWARN(
                    "Not loading Time field for " << threatItr.key()
                                                  << " into threat database as parsing failed with error for "
                                                  << JsonKeys::timestamp << " " << ex.what());
                Common::Telemetry::TelemetryHelper::getInstance().set("corrupt-threat-database", true);
            }

            ThreatDetails details(correlationId, timeStamp);
            tempdatabase.try_emplace(threatItr.key(), details);
        }

        database->swap(tempdatabase);
        LOGINFO("Initialised Threat Database");
    }
} // namespace Plugin