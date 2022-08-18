//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "Common/Threads/AbstractThread.h"

namespace common
{
    class AbstractThreadPluginInterface : public Common::Threads::AbstractThread
    {
    public:
        inline virtual void tryStop() { Common::Threads::AbstractThread::requestStop(); }

    };
}
