// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "unixsocket/BaseServerConnectionThread.h"
#include "unixsocket/ReadBufferAsync.h"
#include "unixsocket/ReadLengthAsync.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

#include <cstdint>
#include <string>

namespace unixsocket
{
    class SafeStoreServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        static constexpr size_t MAXIMUM_MESSAGE_SIZE = 128*1024;
        SafeStoreServerConnectionThread(const SafeStoreServerConnectionThread&) = delete;
        SafeStoreServerConnectionThread& operator=(const SafeStoreServerConnectionThread&) = delete;
        explicit SafeStoreServerConnectionThread(
            datatypes::AutoFd& fd,
            std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager,
            Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCalls);
        void run() override;

    private:
        void inner_run();
        bool read_socket(int socketFd);

        datatypes::AutoFd m_fd;
        std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> m_quarantineManager;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_sysCalls;
        bool loggedLengthOfZero_ = false;
        unixsocket::ReadLengthAsync readLengthAsync_;
        unixsocket::ReadBufferAsync readBufferAsync_;
    };
} // namespace unixsocket
