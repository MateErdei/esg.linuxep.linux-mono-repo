// Copyright 2022, Sophos Limited.  All rights reserved.

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
    class SafeStoreServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        SafeStoreServerConnectionThread(const SafeStoreServerConnectionThread&) = delete;
        SafeStoreServerConnectionThread& operator=(const SafeStoreServerConnectionThread&) = delete;
        explicit SafeStoreServerConnectionThread(
            datatypes::AutoFd& fd,
            std::shared_ptr<IMessageCallback> threatReportCallback);
        void run() override;

    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<IMessageCallback> m_threatReportCallback;
    };
}


