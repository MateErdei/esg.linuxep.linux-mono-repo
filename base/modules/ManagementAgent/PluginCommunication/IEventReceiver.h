///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunication
    {
        class IEventReceiver
        {
        public:
            virtual ~IEventReceiver() = default;

            /**
             * Handle the situation where a plugin is sending an event to the management agent
             *
             * @param appId
             * @param eventXml
             */
            virtual void receivedSendEvent(const std::string& appId, const std::string& eventXml) = 0;
        };
    } // namespace PluginCommunication
} // namespace ManagementAgent
