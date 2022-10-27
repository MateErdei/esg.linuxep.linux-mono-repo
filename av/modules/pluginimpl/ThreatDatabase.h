// Copyright 2022, Sophos Limited.  All rights reserved.
#include <Common/PersistentValue/PersistentValue.h>
#include <map>
#include <list>
namespace Plugin
{

    class ThreatDatabase
    {
    public:
        explicit ThreatDatabase(const std::string& path);
        ~ThreatDatabase();
        void addThreat(const std::string& threatID, const std::string correlationID);
        void removeThreat(const std::string& threatID, const std::string correlationID);

    private:
        void convertDatabaseToString();
        void convertStringToDatabase();
        std::map<std::string,std::list<std::string>> m_database;
        Common::PersistentValue<std::string> m_databaseInString;
    };
}

