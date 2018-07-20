///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_STATUSRECEIVERIMPL_H
#define EVEREST_BASE_STATUSRECEIVERIMPL_H

#include "StatusCache.h"

#include <ManagementAgent/PluginCommunication/IStatusReceiver.h>

namespace ManagementAgent
{
    namespace StatusReceiverImpl
    {
        class StatusReceiverImpl : public virtual ManagementAgent::PluginCommunication::IStatusReceiver
        {
        public:

            void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo& statusInfo) override;
        private:
            StatusCache m_statusCache;
        };
    }
}


#endif //EVEREST_BASE_STATUSRECEIVERIMPL_H
