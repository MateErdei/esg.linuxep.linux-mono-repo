// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/PersistentValue/PersistentValue.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/LoggerPlugin/ResultsSenderInterface.h"
#else
#include "LoggerPlugin/ResultsSenderInterface.h"
#endif



#include <json/value.h>

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
        std::string  intermediaryPath,
        std::string  datafeedPath,
        std::string  osqueryXDRConfigFilePath,
        std::string  osqueryMTRConfigFilePath,
        std::string  osqueryCustomConfigFilePath,
        const std::string& pluginVarDir,
        unsigned long long int dataLimit,
        unsigned int periodInSeconds,
        std::function<void(void)> dataExceededCallback
        );
    std::optional<std::string> PrepareSingleResult(const std::string& result) override;
    void Add(const std::string& result) override;
    // cppcheck-suppress virtualCallInConstructor
    void Send() override;
    void Reset() override;
    void loadScheduledQueryTags();
    void setDataLimit(unsigned long long int limitBytes);
    bool getDataLimitReached();
    unsigned long long int getDataLimit();
    void setDataPeriod(unsigned int periodSeconds);
    void setMtrLicense(bool hasMTRLicense);
    Json::Value PrepareBatchResults() override;
    void SaveBatchResults(const Json::Value& results) override;

    // This needs to be called periodically so that the result sender re-enables itself once the period has elapsed.
    // Returns true if the period has elapsed so that any calling method knows, e.g. plugin adapter
    // (via logger extension) can re-enable the query pack config file if this returns true.
    bool checkDataPeriodElapsed();

    uintmax_t GetFileSize() override;

    // Protected so we can unit test things
protected:
    std::map<std::string, std::pair<std::string, std::string>> m_scheduledQueryTagMap{};

private:
    bool m_firstEntry = true;
    bool m_mtrLicense = false;
    std::string m_intermediaryPath;
    std::string m_datafeedPath;
    std::string m_osqueryXDRConfigFilePath;
    std::string m_osqueryMTRConfigFilePath;
    std::string m_osqueryCustomConfigFilePath;
    Common::PersistentValue<unsigned int> m_currentDataUsage;
    Common::PersistentValue<unsigned int> m_periodStartTimestamp;
    unsigned long long int m_dataLimit;
    Common::PersistentValue<unsigned int> m_periodInSeconds;
    std::function<void(void)> m_dataExceededCallback;

    // This is set to true once we have hit the limit during the period.
    Common::PersistentValue<bool> m_hitLimitThisPeriod;

    Json::Value readJsonFile(const std::string& path);

    void loadScheduledQueryTagsFromFile(std::vector<ScheduledQuery> &scheduledQueries, Path queryPackFilePath);
};