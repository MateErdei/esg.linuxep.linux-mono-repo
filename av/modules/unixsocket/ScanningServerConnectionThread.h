/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <scan_messages/AutoFd.h>
#include <scan_messages/ScanResponse.h>
#include "Common/Threads/NotifyPipe.h"

#include <cstdint>
#include <string>

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
        scan_messages::ScanResponse scan(scan_messages::AutoFd& fd, const std::string& file_path);
    };
}


