/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "AutoFd.h"
#include <ScanRequest.capnp.h>
#include <string>

namespace scan_messages
{

    class ScanRequest
    {
    public:

        using Builder = Sophos::ssplav::FileScanRequest::Builder;
        using Reader = Sophos::ssplav::FileScanRequest::Reader;

        ScanRequest() = default;
        ~ScanRequest();

        /*
         * Accessors for fields from the scan request.
         */
        int fd();
        std::string path();

        /*
         * Receiver side interface
         */
        ScanRequest(int fd, Reader& requestMessage);
        void resetRequest(int fd, Reader& requestMessage);

        /*
         * Sender side interface - set the path and fd, then serialise
         */
        void setPath(const std::string& path);
        void setFd(int);
        std::string serialise();

    private:
        AutoFd m_fd;
        std::string m_path;
        void close();
        void setRequestFromMessage(Reader& requestMessage);
    };
}
