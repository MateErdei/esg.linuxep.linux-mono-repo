// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"
#include "scan_messages/ThreatDetected.h"
#include "unixsocket/IMessageCallback.h"
#include "unixsocket/BaseServerConnectionThread.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"
#include "Common/Threads/NotifyPipe.h"
#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

namespace unixsocket
{
    class ThreatReporterServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        ThreatReporterServerConnectionThread(const ThreatReporterServerConnectionThread&) = delete;
        ThreatReporterServerConnectionThread& operator=(const ThreatReporterServerConnectionThread&) = delete;
        explicit ThreatReporterServerConnectionThread(
            datatypes::AutoFd& fd,
            std::shared_ptr<IMessageCallback> threatReportCallback,
            Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCalls);
        void run() override;

    private:
        void inner_run();

        datatypes::AutoFd m_fd;
        std::shared_ptr<IMessageCallback> m_threatReportCallback;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_sysCalls;
    TEST_PUBLIC:
        std::chrono::milliseconds readTimeout_ = std::chrono::seconds{1};
    };
}


