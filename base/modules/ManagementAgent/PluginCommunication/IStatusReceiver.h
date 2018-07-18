///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef MANAGEMENTAGENT_PLUGINCOMMUNICATION_ISTATUSRECEIVER_H
#define MANAGEMENTAGENT_PLUGINCOMMUNICATION_ISTATUSRECEIVER_H

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
            virtual void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo& statusInfo) = 0;
        };
    }
}

#endif //MANAGEMENTAGENT_PLUGINCOMMUNICATION_ISTATUSRECEIVER_H
