/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include <datatypes/AutoFd.h>
#include <scan_messages/ScanResponse.h>
#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>
#include <unixsocket/IMessageCallback.h>
#include "unixsocket/BaseServerConnectionThread.h"

namespace unixsocket
{
    class ScanningServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        ScanningServerConnectionThread(const ScanningServerConnectionThread&) = delete;
        ScanningServerConnectionThread& operator=(const ScanningServerConnectionThread&) = delete;
        explicit ScanningServerConnectionThread(int fd, std::shared_ptr<IMessageCallback> callback);
        void run() override;

    private:
        datatypes::AutoFd m_fd;
        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const std::string& file_path);
        std::shared_ptr<IMessageCallback> m_callback;
    };
}


