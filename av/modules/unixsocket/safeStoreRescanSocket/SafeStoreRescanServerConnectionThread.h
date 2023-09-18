// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "unixsocket/BaseServerConnectionThread.h"

#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

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

    TEST_PUBLIC:
        std::chrono::milliseconds readTimeout_ = std::chrono::seconds{1};
    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> m_quarantineManager;
    };
} // namespace unixsocket
