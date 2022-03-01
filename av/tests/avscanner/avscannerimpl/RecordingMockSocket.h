/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "common/AbortScanException.h"
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
        bool m_withDetections;
        explicit RecordingMockSocket(const bool withDetections=true)
        : m_withDetections(withDetections)
        {}

        scan_messages::ScanResponse
        scan(datatypes::AutoFd &, const scan_messages::ClientScanRequest &request) override {
            std::string p = ScanPathAccessor(request);
            m_paths.emplace_back(p);
//            PRINT("Scanning " << p);
            scan_messages::ScanResponse response;
            if (m_withDetections)
            {
                response.addDetection(p, "threatName","sha256");
            }
            return response;
        }

        std::vector <std::string> m_paths;
    };

    class AbortingTestSocket : public unixsocket::IScanningClientSocket
    {
    public:
        int m_abortCount = 0;
        scan_messages::ScanResponse scan(
            datatypes::AutoFd&,
            const scan_messages::ClientScanRequest&) override
        {
            m_abortCount++;
            throw AbortScanException("Deliberate Abort");
        }

    };
}
