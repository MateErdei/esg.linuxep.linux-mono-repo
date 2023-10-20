// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/ScanRequest.capnp.h"
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
         * Receiver side interface
         */
        explicit ScanRequest(Reader& requestMessage);
        void resetRequest(Reader& requestMessage);

        /**
         * Should we scan inside archives in this file
         */
         [[nodiscard]] bool scanInsideArchives() const;

         [[nodiscard]] std::string getExecutablePath() const;

         [[nodiscard]] int64_t getPid() const;
        /**
         * Should we scan inside images
         * */
        [[nodiscard]] bool scanInsideImages() const;

        /**
         * Should we detect Potentially Unwanted Applications
         * */
        [[nodiscard]] bool detectPUAs() const;

        /**
         * The path to scan
         */
        [[nodiscard]] std::string path() const;
        bool operator==(const ScanRequest& rhs) const;
    private:
        void close();
        void setRequestFromMessage(Reader& requestMessage);
    };
}
