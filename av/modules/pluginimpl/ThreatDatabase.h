// Copyright 2022, Sophos Limited.  All rights reserved.
#include <Common/PersistentValue/PersistentValue.h>

#include <common/LockableData.h>

#include <map>
#include <list>
namespace Plugin
{

    class ThreatDatabase
    {
    public:
        explicit ThreatDatabase(const std::string& path);
        ~ThreatDatabase();
        void addThreat(const std::string& threatID, const std::string& correlationID);
        void removeCorrelationID(const std::string& correlationID);
        void removeThreatID(const std::string& threatID, bool ignoreNotInDatabase=true);
        void resetDatabase();
        bool isDatabaseEmpty();

    private:
        void convertDatabaseToString();
        void convertStringToDatabase();
        static void setCorruptThreatDatabaseTelemetry(bool corrupt);
        mutable common::LockableData<std::map<std::string,std::list<std::string>>> m_database;
        Common::PersistentValue<std::string> m_databaseInString;
    };
}

