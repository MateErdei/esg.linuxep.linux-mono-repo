// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
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
    class ScanningServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        ScanningServerConnectionThread(const ScanningServerConnectionThread&) = delete;
        ScanningServerConnectionThread& operator=(const ScanningServerConnectionThread&) = delete;
        explicit ScanningServerConnectionThread(
                datatypes::AutoFd& fd,
                threat_scanner::IThreatScannerFactorySharedPtr scannerFactory,
            Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCalls,
                int maxIterations = -1);
        void run() override;

    TEST_PUBLIC:
        /**
         * @returns false if shouldnt continue trying to scan
        */
        bool attemptScan(
            std::shared_ptr<scan_messages::ScanRequest> scanRequest,
            std::string& errMsg,
            scan_messages::ScanResponse& result,
            datatypes::AutoFd& socket_fd);

        std::chrono::milliseconds readTimeout_ = std::chrono::seconds{1};

    private:
        void inner_run();
        bool sendResponse(datatypes::AutoFd& socket_fd, const scan_messages::ScanResponse& response);

        datatypes::AutoFd socketFd_;
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory_;
        threat_scanner::IThreatScannerPtr scanner_;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCalls_;

        int maxIterations_;
    };

    struct ScanRequestObject
    {
        std::string pathname;
        bool scanArchives;
        int64_t scanType;
        std::string userID;
    };
}


