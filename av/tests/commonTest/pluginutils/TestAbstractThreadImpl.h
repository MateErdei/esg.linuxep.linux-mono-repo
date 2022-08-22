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

    private:
        void run() override
        {
            announceThreadStarted();
        }

        void tryStop() override
        {
            LOGDEBUG("Request Stop in TestAbstractThreadImpl");
            Common::Threads::AbstractThread::requestStop();
        }
    };
}