//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "Common/Threads/AbstractThread.h"

namespace common
{
    class AbstractThreadPluginInterface : public Common::Threads::AbstractThread
    {
    public:
        inline virtual void tryStop() { Common::Threads::AbstractThread::requestStop(); }

        void clearTerminationPipe()
        {
            while(m_notifyPipe.notified())
            {}
        }

        /**
         * Start or restart the thread, ensuring termination pipe is cleared
         */
        void restart()
        {
            clearTerminationPipe();
            start();
        }

    };
}
