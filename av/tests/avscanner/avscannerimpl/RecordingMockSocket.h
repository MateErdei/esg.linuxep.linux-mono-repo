/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "scan_messages/ClientScanRequest.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include "common/AbortScanException.h"

#include <fcntl.h>

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
        explicit RecordingMockSocket(const bool withDetections=true)
        : m_withDetections(withDetections)
        {
            // socket needs to be something that will not hang in a ::poll call.
            m_socketFd.reset(::open("/dev/zero", O_RDONLY));
        }


        int connect() override
        {
            return 0;
        }

        bool sendRequest(datatypes::AutoFd&, const scan_messages::ClientScanRequest& request) override
        {
            std::string p = ScanPathAccessor(request);
            m_paths.emplace_back(p);
            //            PRINT("Scanning " << p);
            return true;
        }

        bool receiveResponse(scan_messages::ScanResponse& response) override
        {
            if (m_withDetections)
            {
                response.addDetection(m_paths.back(), "threatName","sha256");
            }
            return true;
        }

        int socketFd() override
        {
            return m_socketFd.fd();
        }

        bool m_withDetections;
        std::vector <std::string> m_paths;
        datatypes::AutoFd m_socketFd;
    };

    class AbortingTestSocket : public unixsocket::IScanningClientSocket
    {
    public:
        int connect() override
        {
            // real connect() method never throws.
            return 0;
        }

        bool sendRequest(datatypes::AutoFd&, const scan_messages::ClientScanRequest&) override
        {
            m_abortCount++;
            throw common::AbortScanException("Deliberate Abort");
        }

        bool receiveResponse(scan_messages::ScanResponse&) override
        {
            m_abortCount++;
            throw common::AbortScanException("Deliberate Abort");
        }

        int socketFd() override
        {
            return -1;
        }

        int m_abortCount = 0;
    };
}
