///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

#include <Common/PluginApi/StatusInfo.h>

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunication
    {
        class IStatusReceiver
        {
        public:
            virtual ~IStatusReceiver() = default;

            /**
             * Handle the situation where a plugin is sending a changed status to the management agent
             *
             * @param appId
             * @param statusInfo
             */
            virtual void receivedChangeStatus(
                const std::string& appId,
                const Common::PluginApi::StatusInfo& statusInfo) = 0;
        };
    } // namespace PluginCommunication
} // namespace ManagementAgent
