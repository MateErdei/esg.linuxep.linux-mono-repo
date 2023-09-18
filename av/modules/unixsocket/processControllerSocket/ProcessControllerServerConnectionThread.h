// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "unixsocket/BaseServerConnectionThread.h"
#include "IProcessControlMessageCallback.h"

#include "Common/Threads/AbstractThread.h"
#include "Common/Threads/NotifyPipe.h"

#include "datatypes/AutoFd.h"
#include "scan_messages/ThreatDetected.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

#include <cstdint>
#include <string>

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

namespace unixsocket
{
    class ProcessControllerServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        ProcessControllerServerConnectionThread(const ProcessControllerServerConnectionThread&) = delete;
        ProcessControllerServerConnectionThread& operator=(const ProcessControllerServerConnectionThread&) = delete;
        explicit ProcessControllerServerConnectionThread(
            datatypes::AutoFd& fd,
            std::shared_ptr<IProcessControlMessageCallback> processControlCallback,
            Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCalls
            );

        void run() override;

    TEST_PUBLIC:
        std::chrono::milliseconds readTimeout_ = std::chrono::seconds{1};
    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<IProcessControlMessageCallback> m_controlMessageCallback;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_sysCalls;
    };
}


