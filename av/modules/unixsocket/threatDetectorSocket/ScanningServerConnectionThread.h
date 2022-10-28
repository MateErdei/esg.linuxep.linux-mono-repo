// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
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
                int maxIterations = -1);
        void run() override;

    private:

        void inner_run();
        bool sendResponse(datatypes::AutoFd& socket_fd, const scan_messages::ScanResponse& response);
        bool isReceivedFdFile(datatypes::AutoFd& file_fd, std::string& errMsg);
        bool isReceivedFileOpen(datatypes::AutoFd& file_fd, std::string& errMsg);
        bool readCapnProtoMsg(
            int32_t length,
            uint32_t& buffer_size,
            kj::Array<capnp::word>& proto_buffer,
            datatypes::AutoFd& socket_fd,
            ssize_t& bytes_read,
            bool& loggedLengthOfZero,
            std::string& errMsg);

        datatypes::AutoFd m_socketFd;
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
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


