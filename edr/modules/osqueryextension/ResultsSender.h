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
    ResultsSender();
    ~ResultsSender();
    void Add(const std::string &result) override;
    void Send() override;
    void Reset() override;
    uintmax_t GetFileSize() override;

private:
    bool m_firstEntry = true;

    // TODO fix these up by passing them in to result sender
    inline static const std::string INTERMEDIARY_PATH = "/opt/sophos-spl/plugins/edr/var/tmp_file";
    inline static const std::string DATAFEED_PATH = "/opt/sophos-spl/base/mcs/datafeed";
    std::string m_osqueryXDRConfigFilePath = "/opt/sophos-spl/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf";


    std::vector<ScheduledQuery> m_scheduledQueryTags;
    Json::Value readJsonFile(const std::string& path);
    void loadScheduledQueryTags();
};