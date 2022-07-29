/******************************************************************************************************

Copyright 2020-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Threads/AbstractThread.h"

namespace common
{
    class ThreadRunner
    {
    public:
        explicit ThreadRunner(Common::Threads::AbstractThread &thread, std::string name);

        ~ThreadRunner();

    private:
        void killThreads();

        Common::Threads::AbstractThread &m_thread;
        std::string m_name;
    };
}