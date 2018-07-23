///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef MANAGEMENTAGENT_EVENTRECEIVERIMPL_EVENTRECEIVERIMPL_H
#define MANAGEMENTAGENT_EVENTRECEIVERIMPL_EVENTRECEIVERIMPL_H


#include <ManagementAgent/PluginCommunication/IEventReceiver.h>
#include <Common/TaskQueue/ITaskQueue.h>

namespace ManagementAgent
{
    namespace EventReceiverImpl
    {
        class EventReceiverImpl
                    : public virtual ManagementAgent::PluginCommunication::IEventReceiver
        {
        public:
            explicit EventReceiverImpl(
                    std::string mcsDir,
                    Common::TaskQueue::ITaskQueue& taskQueue);

            void receivedSendEvent(const std::string& appId, const std::string &eventXml) override;
        private:
            std::string m_mcsDir;
            Common::TaskQueue::ITaskQueue& m_taskQueue;
        };

    }
}


#endif //MANAGEMENTAGENT_EVENTRECEIVERIMPL_EVENTRECEIVERIMPL_H
