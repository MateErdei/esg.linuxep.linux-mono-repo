/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <cstdint>
#include "Common/Threads/NotifyPipe.h"

namespace unixsocket
{
    class ScanningServerConnectionThread
    {
    public:
        explicit ScanningServerConnectionThread(int fd);
        void run();
        void notifyTerminate();
        bool finished();
        void start();
    private:
        bool m_finished;
        int m_fd;
        Common::Threads::NotifyPipe m_terminationPipe;
    };
}


