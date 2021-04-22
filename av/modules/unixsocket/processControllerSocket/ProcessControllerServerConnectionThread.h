/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include <datatypes/AutoFd.h>
#include <scan_messages/ThreatDetected.h>
#include "Common/Threads/NotifyPipe.h"
#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>
#include <unixsocket/IMessageCallback.h>
#include "unixsocket/BaseServerConnectionThread.h"

namespace unixsocket
{
    class ProcessControllerServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        ProcessControllerServerConnectionThread(const ProcessControllerServerConnectionThread&) = delete;
        ProcessControllerServerConnectionThread& operator=(const ProcessControllerServerConnectionThread&) = delete;
        explicit ProcessControllerServerConnectionThread(
            datatypes::AutoFd& fd, std::shared_ptr<Common::Threads::NotifyPipe> shutdownPipe);
        void run() override;

    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<Common::Threads::NotifyPipe> m_shutdownPipe;
    };
}


