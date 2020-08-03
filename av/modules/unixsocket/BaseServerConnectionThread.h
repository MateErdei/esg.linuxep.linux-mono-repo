/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Threads/AbstractThread.h"


namespace unixsocket
{
    class BaseServerConnectionThread : public Common::Threads::AbstractThread
    {
    public:
        bool isRunning() { return m_isRunning; }

    protected:
        bool m_isRunning = false;
    };
}