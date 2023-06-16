// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"
#include "scan_messages/ScanRequest.h"
#include "scan_messages/ScanResponse.h"
#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"

#include "unixsocket/BaseServerConnectionThread.h"
#include "unixsocket/IMessageCallback.h"

#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>

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
                datatypes::ISystemCallWrapperSharedPtr sysCalls,
                int maxIterations = -1);
        void run() override;

    TEST_PUBLIC:
        //returns false if shouldnt continue
        bool attemptScan(
            std::shared_ptr<scan_messages::ScanRequest> scanRequest,
            std::string& errMsg,
            scan_messages::ScanResponse& result,
            datatypes::AutoFd& socket_fd);

    private:
        void inner_run();
        bool sendResponse(datatypes::AutoFd& socket_fd, const scan_messages::ScanResponse& response);

        datatypes::AutoFd m_socketFd;
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
        threat_scanner::IThreatScannerPtr m_scanner;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;

        int m_maxIterations;
    };

    struct ScanRequestObject
    {
        std::string pathname;
        bool scanArchives;
        int64_t scanType;
        std::string userID;
    };
}


