// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "scan_messages/MetadataRescan.h"
#include "scan_messages/ScanRequest.h"
#include "scan_messages/ScanResponse.h"
#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"
#include "unixsocket/BaseServerConnectionThread.h"
#include "unixsocket/IMessageCallback.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"
#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

namespace unixsocket
{
    class MetadataRescanServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        MetadataRescanServerConnectionThread(const MetadataRescanServerConnectionThread&) = delete;
        MetadataRescanServerConnectionThread& operator=(const MetadataRescanServerConnectionThread&) = delete;
        explicit MetadataRescanServerConnectionThread(
            datatypes::AutoFd& fd,
            threat_scanner::IThreatScannerFactorySharedPtr scannerFactory,
            Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCalls,
            int maxIterations = -1);
        void run() override;

    TEST_PUBLIC:
        std::chrono::milliseconds readTimeout_ = std::chrono::seconds{1};
    private:
        void inner_run();
        bool sendResponse(datatypes::AutoFd& socket_fd, const scan_messages::MetadataRescanResponse& response);

        datatypes::AutoFd m_socketFd;
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_sysCalls;
        int m_maxIterations;
    };
} // namespace unixsocket
