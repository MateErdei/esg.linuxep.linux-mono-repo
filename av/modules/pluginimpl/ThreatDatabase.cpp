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

    void ThreatDatabase::addThreat(const std::string& threatID, const std::string correlationID)
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
            LOGWARN("failed to parse threat database on disk with error: "<< ex.what() << " resetting database");
        }
        std::map<std::string,std::list<std::string>> tempdatabase;
        for (const auto& key : j)
        {
            std::list<std::string> correlationids;
            std::string threatid = key;
            nlohmann::json ids = j[threatid];
            for(const auto& id : j)
            {
                correlationids.emplace_back(id);
            }
            tempdatabase.emplace(threatid, correlationids);
        }
        m_database = tempdatabase;
    }
}