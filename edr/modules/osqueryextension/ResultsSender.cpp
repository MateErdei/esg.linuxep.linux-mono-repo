/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ResultsSender.h"

#include "Logger.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <iostream>

ResultsSender::ResultsSender()
{
    LOGDEBUG("Created results sender");
}

ResultsSender::~ResultsSender()
{
    Send();
}

void ResultsSender::Add(const std::string& result)
{
    LOGDEBUG("Adding XDR results to intermediary file: " << result);

    std::string stringToAppend;
    if (!m_firstEntry)
    {
        stringToAppend =",";
    }

    stringToAppend.append(result);

    auto filesystem = Common::FileSystem::fileSystem();
    filesystem->appendFile(INTERMEDIARY_PATH, stringToAppend);

    m_firstEntry = false;
}

void ResultsSender::Send()
{
    LOGDEBUG("Send XDR Results");
    auto filesystem = Common::FileSystem::fileSystem();
    if (filesystem->exists(INTERMEDIARY_PATH))
    {
        filesystem->appendFile(INTERMEDIARY_PATH, "]");
        auto filepermissions = Common::FileSystem::filePermissions();
        filepermissions->chown(INTERMEDIARY_PATH, "sophos-spl-local", "sophos-spl-group");
        filepermissions->chmod(INTERMEDIARY_PATH, 0640);
        Common::UtilityImpl::FormattedTime time;
        std::string now = time.currentEpochTimeInSeconds();
        std::stringstream fileName;
        fileName << std::string("scheduled_query-") << now << ".json";
        Path outputFilePath = Common::FileSystem::join(DATAFEED_PATH, fileName.str());

        LOGINFO("Sending batched XDR scheduled query results - " << outputFilePath);

        filesystem->moveFile(INTERMEDIARY_PATH, outputFilePath);
    }
}

void ResultsSender::Reset()
{
    LOGDEBUG("Reset");
    auto filesystem = Common::FileSystem::fileSystem();
    if (filesystem->exists(INTERMEDIARY_PATH))
    {
        filesystem->removeFile(INTERMEDIARY_PATH);
    }
    filesystem->appendFile(INTERMEDIARY_PATH, "[");
    m_firstEntry = true;
}

uintmax_t ResultsSender::GetFileSize()
{
    LOGDEBUG("GetFileSize, returning 0 for now.");
    return 0;
}
