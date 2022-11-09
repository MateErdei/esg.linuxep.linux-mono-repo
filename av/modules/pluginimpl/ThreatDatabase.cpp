// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ThreatDatabase.h"

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
        std::map<std::string,std::list<std::string>>::iterator it = m_database.find(threatID);
        if (it != m_database.end())
        {
            std::list oldListIDS = it->second;
            if (std::find(oldListIDS.begin(), oldListIDS.end(), correlationID) == oldListIDS.end() )
            {
                it->second.emplace_back(correlationID);
            }
        }
        else
        {
            std::list<std::string> ids;
            ids.emplace_back(correlationID);
            m_database.emplace(threatID,ids);
        }

    }

    void ThreatDatabase::removeCorrelationID(const std::string& correlationID)
    {
        std::string threatID = "";
        for (auto const& entry : m_database)
        {
            std::list oldListIDS = entry.second;
            auto iterator = std::find(oldListIDS.begin(), oldListIDS.end(), correlationID);
            if (iterator != oldListIDS.end() )
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
            std::map<std::string,std::list<std::string>>::iterator it = m_database.find(threatID);
            m_database.erase(it);
            LOGDEBUG("Removed threat from database");
        }
    }

    void ThreatDatabase::removeThreatID(const std::string& threatID, bool ignoreNotInDatabase)
    {
        std::map<std::string,std::list<std::string>>::iterator it = m_database.find(threatID);
        if (it != m_database.end())
        {
            m_database.erase(it);
            LOGDEBUG("Removed threat id " << threatID << " from database");
        }
        else if (!ignoreNotInDatabase)
        {
            LOGWARN("Cannot remove threat id" << threatID << " from database as it cannot be found");
        }
    }

    bool ThreatDatabase::isDatabaseEmpty()
    {
        return m_database.empty();
    }

    void ThreatDatabase::resetDatabase()
    {
        std::map<std::string,std::list<std::string>> tempdatabase;
        m_database = tempdatabase;
        m_databaseInString.setValueAndForceStore("{}");
    }

    void ThreatDatabase::convertDatabaseToString()
    {
        nlohmann::json j;
        for (const auto& key: m_database)
        {
            j[key.first] = key.second;
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
        nlohmann::json j;

        try
        {
            j = nlohmann::json::parse(m_databaseInString.getValue());
        }
        catch (nlohmann::json::exception& ex)
        {
            LOGWARN("Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error: " << ex.what());
        }

        std::map<std::string,std::list<std::string>> tempdatabase;
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            std::list<std::string> correlationids;
            try
            {
                for (auto it2 = it.value().begin(); it2 != it.value().end(); ++it2)
                {
                    correlationids.emplace_back(it2.value());
                }
                tempdatabase.emplace(it.key(), correlationids);
            }
            catch (nlohmann::json::exception& ex)
            {
                //If the types of the threat id or correlation id are wrong throw away the entire threatID entry
                LOGWARN("Not loading "<< it.key() << " into threat database as the parsing failed with error " << ex.what());
            }
        }
        m_database = tempdatabase;
        LOGINFO("Initialised Threat Database");
    }
}