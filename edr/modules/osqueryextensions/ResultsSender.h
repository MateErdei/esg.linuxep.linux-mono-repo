/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/PersistentValue/PersistentValue.h>
#include <redist/jsoncpp/include/json/value.h>

#include <ResultsSenderInterface.h>

struct ScheduledQuery
{
    std::string queryNameWithPack;
    std::string queryName;
    std::string tag;
};

class ResultsSender : public ResultsSenderInterface
{
public:
    ResultsSender(
        const std::string& intermediaryPath,
        const std::string& datafeedPath,
        const std::string& osqueryXDRConfigFilePath,
        const std::string& pluginVarDir,
        unsigned int dataLimit,
        unsigned int periodInSeconds);
    ~ResultsSender();
    void Add(const std::string& result) override;
    void Send() override;
    void Reset() override;
    void setDataLimit(unsigned int limitBytes);
    void setDataPeriod(unsigned int periodSeconds);
    uintmax_t GetFileSize() override;

    // Protected so we can unit test things
protected:
    std::vector<ScheduledQuery> m_scheduledQueryTags;

    std::map<std::string, std::pair<std::string, std::string>> getQueryTagMap();

private:
    bool m_firstEntry = true;
    std::string m_intermediaryPath;
    std::string m_datafeedPath;
    std::string m_osqueryXDRConfigFilePath;
    Common::PersistentValue<unsigned int> m_currentDataUsage;
    Common::PersistentValue<unsigned int> m_periodStartTimestamp;
    unsigned int m_dataLimit;
    unsigned int m_periodInSeconds;

    // This is set to true once we have hit the limit during the period so we limit number of statuses and logs.
    // This is done like this to allows us to still send up small results after a potentially large result that would
    // have pushed us over the limit
    Common::PersistentValue<bool> m_hitLimitThisPeriod;

    Json::Value readJsonFile(const std::string& path);
    void loadScheduledQueryTags();
};