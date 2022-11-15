// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "unixsocket/BaseServerConnectionThread.h"

#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>

namespace unixsocket
{
    class SafeStoreRescanServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        SafeStoreRescanServerConnectionThread(const SafeStoreRescanServerConnectionThread&) = delete;
        SafeStoreRescanServerConnectionThread& operator=(const SafeStoreRescanServerConnectionThread&) = delete;
        explicit SafeStoreRescanServerConnectionThread(
            datatypes::AutoFd& fd,
            std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager);
        void run() override;

    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> m_quarantineManager;
    };
} // namespace unixsocket
