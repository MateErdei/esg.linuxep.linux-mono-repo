/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
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
    ResultsSender(const std::string& intermediaryPath, const std::string& datafeedPath, const std::string& osqueryXDRConfigFilePath);
    ~ResultsSender();
    void Add(const std::string &result) override;
    void Send() override;
    void Reset() override;
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

    Json::Value readJsonFile(const std::string& path);
    void loadScheduledQueryTags();
};