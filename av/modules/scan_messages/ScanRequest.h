/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScanRequest.capnp.h"
#include "ClientScanRequest.h"

#include "datatypes/AutoFd.h"

#include <string>

namespace scan_messages
{

    class ScanRequest : public ClientScanRequest
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

        /*
         * Receiver side interface
         */
        ScanRequest(int fd, Reader& requestMessage);
        void resetRequest(int fd, Reader& requestMessage);

        /**
         * Should we scan inside archives in this file
         */
         [[nodiscard]] bool scanInsideArchives() const;


        [[nodiscard]] std::string path() const;
    private:
        datatypes::AutoFd m_fd;
        void close();
        void setRequestFromMessage(Reader& requestMessage);
    };
}
