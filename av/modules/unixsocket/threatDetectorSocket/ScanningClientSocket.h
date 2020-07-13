/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IScanningClientSocket.h"

#include "scan_messages/ScanResponse.h"
#include "scan_messages/ClientScanRequest.h"

#include <string>

namespace unixsocket
{
    class ScanningClientSocket : public IScanningClientSocket
    {
    public:
        ScanningClientSocket& operator=(const ScanningClientSocket&) = delete;
        ScanningClientSocket(const ScanningClientSocket&) = delete;
        explicit ScanningClientSocket(const std::string& socket_path);
        ~ScanningClientSocket() = default;

        scan_messages::ScanResponse scan(int fd, const std::string& file_path);
        scan_messages::ScanResponse scan(
            datatypes::AutoFd& fd,
            const std::string& file_path,
            bool scanInsideArchives=false,
            scan_messages::E_SCAN_TYPE scanType=scan_messages::E_SCAN_TYPE_ON_DEMAND,
            const std::string& userID="root");
        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest&) override;

    private:
        datatypes::AutoFd m_socket_fd;
    };
}
