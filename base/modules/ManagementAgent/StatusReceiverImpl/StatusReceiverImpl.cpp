///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include "StatusReceiverImpl.h"

#include <Common/FileSystem/IFileSystem.h>

ManagementAgent::StatusReceiverImpl::StatusReceiverImpl::StatusReceiverImpl(const std::string& mcsDir)
{
    m_tempDir = Common::FileSystem::fileSystem()->join(mcsDir,"tmp");
    m_statusDir = Common::FileSystem::fileSystem()->join(mcsDir,"status");
}

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

void ManagementAgent::StatusReceiverImpl::StatusReceiverImpl::receivedChangeStatus(const std::string& appId,
                                                                                   const Common::PluginApi::StatusInfo& statusInfo)
{
    if (m_statusCache.statusChanged(appId,statusInfo.statusWithoutTimestampsXml))
    {
        Path basename = createStatusFilename(appId);
        Path filepath = Common::FileSystem::fileSystem()->join(m_statusDir,basename);
        // write file to directory
        Common::FileSystem::fileSystem()->writeFileAtomically(filepath,statusInfo.statusXml,m_tempDir);
    }
}
