/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "GenericShutdownListener.h"

namespace Common
{
    namespace ReactorImpl
    {
        void GenericShutdownListener::notifyShutdownRequested() { m_callback(); }

        GenericShutdownListener::GenericShutdownListener(std::function<void()> callback)
        {
            if (callback)
            {
                m_callback = callback;
            }
            else
            {
                m_callback = []() {};
            }
        }
    } // namespace ReactorImpl
} // namespace Common