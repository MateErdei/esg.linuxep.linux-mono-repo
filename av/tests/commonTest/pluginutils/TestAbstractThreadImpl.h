/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "common/AbstractThreadPluginInterface.h"
#include "common/Logger.h"

namespace
{
    class TestAbstractThreadImpl : public common::AbstractThreadPluginInterface
    {
    public:
        explicit TestAbstractThreadImpl(const std::string& threadName = "TestAbstractThreadImpl")
        {
            m_threadName = threadName;
        }

    private:
        void run() override
        {
            announceThreadStarted();
        }

        void tryStop() override
        {
            LOGDEBUG("Request Stop in " << m_threadName);
            Common::Threads::AbstractThread::requestStop();
        }

        std::string m_threadName = "";
    };
}