///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "StatusReceiverImpl.h"

void ManagementAgent::StatusReceiverImpl::StatusReceiverImpl::receivedChangeStatus(const std::string& appId,
                                                                                   const Common::PluginApi::StatusInfo& statusInfo)
{
    if (m_statusCache.statusChanged(appId,statusInfo.statusWithoutTimestampsXml))
    {
        // write file to directory
    }
}
