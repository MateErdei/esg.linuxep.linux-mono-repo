///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef MANAGEMENTAGENT_STATUSRECEIVERIMPL_STATUSRECEIVERIMPL_H
#define MANAGEMENTAGENT_STATUSRECEIVERIMPL_STATUSRECEIVERIMPL_H

#include "StatusCache.h"

#include <ManagementAgent/PluginCommunication/IStatusReceiver.h>

namespace ManagementAgent
{
    namespace StatusReceiverImpl
    {
        class StatusReceiverImpl : public virtual ManagementAgent::PluginCommunication::IStatusReceiver
        {
        public:
            explicit StatusReceiverImpl(const std::string& mcsDir);

            void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo& statusInfo) override;
        private:
            StatusCache m_statusCache;
            std::string m_tempDir;
            std::string m_statusDir;
        };
    }
}


#endif //MANAGEMENTAGENT_STATUSRECEIVERIMPL_STATUSRECEIVERIMPL_H
