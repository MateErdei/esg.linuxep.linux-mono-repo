// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "unixsocket/BaseServerConnectionThread.h"
#include "IProcessControlMessageCallback.h"

#include "Common/Threads/AbstractThread.h"
#include "Common/Threads/NotifyPipe.h"

#include <datatypes/AutoFd.h>
#include <datatypes/ISystemCallWrapper.h>
#include <scan_messages/ThreatDetected.h>

#include <cstdint>
#include <string>

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
            datatypes::ISystemCallWrapperSharedPtr sysCalls
            );

        void run() override;

    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<IProcessControlMessageCallback> m_controlMessageCallback;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
    };
}


