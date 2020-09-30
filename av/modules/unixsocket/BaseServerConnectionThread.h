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
        [[nodiscard]] bool isRunning() const;
    protected:
        void setIsRunning(bool value);
    private:
        bool m_isRunning = false;
    };
}