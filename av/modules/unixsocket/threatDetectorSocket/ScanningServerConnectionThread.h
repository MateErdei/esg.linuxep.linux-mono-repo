/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include <datatypes/AutoFd.h>
#include <scan_messages/ScanResponse.h>
#include "Common/Threads/NotifyPipe.h"

#include <cstdint>
#include <string>

namespace unixsocket
{
    class ScanningServerConnectionThread
    {
    public:
        ScanningServerConnectionThread(const ScanningServerConnectionThread&) = delete;
        ScanningServerConnectionThread& operator=(const ScanningServerConnectionThread&) = delete;
        explicit ScanningServerConnectionThread(int fd);
        void run();
        void notifyTerminate();
        bool finished();
        void start();
    private:
        bool m_finished;
        datatypes::AutoFd m_fd;
        Common::Threads::NotifyPipe m_terminationPipe;
        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const std::string& file_path);
    };
}


