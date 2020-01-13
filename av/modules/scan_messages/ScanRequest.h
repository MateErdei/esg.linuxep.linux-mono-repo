/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ScanRequest.capnp.h>
#include <string>

namespace scan_messages
{
    class ScanRequest
    {
    public:
        ScanRequest();
        ScanRequest(int fd, Sophos::ssplav::FileScanRequest::Reader& requestMessage);
        ~ScanRequest();
        void resetRequest(int fd, Sophos::ssplav::FileScanRequest::Reader& requestMessage);

        int fd();
        std::string path();

    private:
        int m_fd;
        std::string m_path;
        void close();
        void setRequestFromMessage(Sophos::ssplav::FileScanRequest::Reader& requestMessage);
    };
}
