/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/TaskQueue/ITask.h>
#include <string>

namespace ManagementAgent
{
    namespace EventReceiverImpl
    {
        class EventTask
            : public virtual Common::TaskQueue::ITask
        {
        public:
            EventTask(
                    std::string appId,
                    std::string eventXml
                    );
            void run() override;
        private:
            std::string m_appId;
            std::string m_eventXml;
        };
    }
}



