/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ResultsSenderInterface.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <thirdparty/nlohmann-json/json.hpp>
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
    inline static const std::string INTERMEDIARY_PATH = "/opt/sophos-spl/plugins/edr/var/tmp_file";
    inline static const std::string DATAFEED_PATH = "/opt/sophos-spl/base/mcs/datafeed";
};