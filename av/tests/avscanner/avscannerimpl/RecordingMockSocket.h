/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "scan_messages/ClientScanRequest.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

namespace
{
    class ScanPathAccessor : public scan_messages::ClientScanRequest
    {
    public:
        ScanPathAccessor(const ClientScanRequest& other) // NOLINT
                : scan_messages::ClientScanRequest(other)
        {}

        operator std::string() const // NOLINT
        {
            return m_path;
        }
    };

    class RecordingMockSocket : public unixsocket::IScanningClientSocket {
    public:
        scan_messages::ScanResponse
        scan(datatypes::AutoFd &, const scan_messages::ClientScanRequest &request) override {
            std::string p = ScanPathAccessor(request);
            m_paths.emplace_back(p);
//            PRINT("Scanning " << p);
            scan_messages::ScanResponse response;
            response.addDetection(p, "");
            return response;
        }

        std::vector <std::string> m_paths;
    };
}
