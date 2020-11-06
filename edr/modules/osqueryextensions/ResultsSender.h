/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/PersistentValue/PersistentValue.h>
#include <redist/jsoncpp/include/json/value.h>
#include <ResultsSenderInterface.h>
#include <functional>

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
        unsigned int periodInSeconds
        ,std::function<void(void)> dataExceededCallback
        );
    ~ResultsSender();
    void Add(const std::string& result) override;
    void Send() override;
    void Reset() override;
    void setDataLimit(unsigned int limitBytes);
    bool getDataLimitReached();
    void setDataPeriod(unsigned int periodSeconds);

    // This needs to be called periodically so that the result sender re-enables itself once the period has elapsed.
    // Returns true if the period has elapsed so that any calling method knows, e.g. plugin adapter
    // (via logger extension) can re-enable the query pack config file if this returns true.
    bool checkDataPeriodElapsed();

    uintmax_t GetFileSize() override;

    // Protected so we can unit test things
protected:
    std::vector<ScheduledQuery> m_scheduledQueryTags{};

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
    std::function<void(void)> m_dataExceededCallback;

    // This is set to true once we have hit the limit during the period.
    Common::PersistentValue<bool> m_hitLimitThisPeriod;

    Json::Value readJsonFile(const std::string& path);
    void loadScheduledQueryTags();
};