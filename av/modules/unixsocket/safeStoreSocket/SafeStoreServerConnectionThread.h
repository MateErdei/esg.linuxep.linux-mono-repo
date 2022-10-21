// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "unixsocket/BaseServerConnectionThread.h"

#include "Common/Threads/AbstractThread.h"
#include "Common/Threads/NotifyPipe.h"

#include <datatypes/AutoFd.h>
#include <scan_messages/ThreatDetected.h>
#include <scan_messages/QuarantineResponse.h>
#include <unixsocket/IMessageCallback.h>

#include <cstdint>
#include <string>

namespace unixsocket
{
    class SafeStoreServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        SafeStoreServerConnectionThread(const SafeStoreServerConnectionThread&) = delete;
        SafeStoreServerConnectionThread& operator=(const SafeStoreServerConnectionThread&) = delete;
        explicit SafeStoreServerConnectionThread(
            datatypes::AutoFd& fd,
            std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager);
        void run() override;

    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> m_quarantineManager;
    };
} // namespace unixsocket
