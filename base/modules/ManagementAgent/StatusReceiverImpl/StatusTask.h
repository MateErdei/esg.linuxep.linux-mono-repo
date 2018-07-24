/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef MANAGEMENTAGENT_STATUSRECEIVERIMPL_STATUSTASK_H
#define MANAGEMENTAGENT_STATUSRECEIVERIMPL_STATUSTASK_H


#include <Common/TaskQueue/ITask.h>
#include "StatusCache.h"

namespace ManagementAgent
{
    namespace StatusReceiverImpl
    {
        class StatusTask
                : public virtual Common::TaskQueue::ITask
        {
        public:
            StatusTask(
                    StatusCache& statusCache,
                    std::string appId,
                    std::string statusXml,
                    std::string statusXmlWithoutTimestamps,
                    std::string tempDir,
                    std::string statusDir
                    );

            void run() override;
        private:
            StatusCache& m_statusCache;
            std::string m_appId;
            std::string m_statusXml;
            std::string m_statusXmlWithoutTimestamps;
            std::string m_statusDir;
            std::string m_tempDir;
        };
    }
}


#endif //MANAGEMENTAGENT_STATUSRECEIVERIMPL_STATUSTASK_H
