/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "GenericShutdownListener.h"

namespace Common
{
    namespace Reactor
    {

        void GenericShutdownListener::notifyShutdownRequested()
        {
            m_callback();
        }

        GenericShutdownListener::GenericShutdownListener(std::function<void()> callback)
        {
            m_callback = callback;
        }
    }
}