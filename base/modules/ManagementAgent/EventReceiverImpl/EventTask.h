/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef MANAGEMENTAGENT_EVENTRECEIVERIMPL_EVENTTASK_H
#define MANAGEMENTAGENT_EVENTRECEIVERIMPL_EVENTTASK_H


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
                    std::string mcsDir,
                    std::string appId,
                    std::string eventXml
                    );
            void run() override;
        private:
            std::string m_mcsDir;
            std::string m_appId;
            std::string m_eventXml;
        };
    }
}


#endif //MANAGEMENTAGENT_EVENTRECEIVERIMPL_EVENTTASK_H
