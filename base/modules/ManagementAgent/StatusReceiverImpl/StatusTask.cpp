/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StatusTask.h"

#include "Logger.h"
#include <Common/FileSystem/IFileSystem.h>

#include <cassert>

namespace
{
    /**
     * Generate <appId>_status.xml
     * @param appId
     * @return
     */
    Path createStatusFilename(const std::string& appId)
    {
        return appId+"_status.xml";
    }
}

void ManagementAgent::StatusReceiverImpl::StatusTask::run()
{
    if (m_statusCache.statusChanged(m_appId,m_statusXmlWithoutTimestamps))
    {
        LOGSUPPORT("Send updated status to mcsrouter from appid: " << m_appId);
        Path basename = createStatusFilename(m_appId);
        assert(!basename.empty());
        Path filepath = Common::FileSystem::fileSystem()->join(m_statusDir,basename);
        assert(!filepath.empty());
        // write file to directory
        Common::FileSystem::fileSystem()->writeFileAtomically(filepath,m_statusXml,m_tempDir);
    }
}

ManagementAgent::StatusReceiverImpl::StatusTask::StatusTask(
        ManagementAgent::StatusReceiverImpl::StatusCache& statusCache, std::string appId, std::string statusXml,
        std::string statusXmlWithoutTimestamps, std::string tempDir, std::string statusDir)
    : m_statusCache(statusCache),
      m_appId(std::move(appId)),
      m_statusXml(std::move(statusXml)),
      m_statusXmlWithoutTimestamps(std::move(statusXmlWithoutTimestamps)),
      m_statusDir(std::move(statusDir)),
      m_tempDir(std::move(tempDir))
{
}
