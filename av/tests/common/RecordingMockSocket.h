//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "scan_messages/ClientScanRequest.h"
#include "sophos_on_access_process/onaccessimpl/ClientSocketException.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include "common/AbortScanException.h"
#include "common/ScanInterruptedException.h"

#include <fcntl.h>

namespace
{
    class RecordingMockSocket : public unixsocket::IScanningClientSocket {
    public:
        explicit RecordingMockSocket(const bool withDetections=true, const bool withErrors=false)
        : m_withDetections(withDetections)
        , m_withErrors(withErrors)
        {
            // socket needs to be something that will not hang in a ::poll call.
            m_socketFd.reset(::open("/dev/zero", O_RDONLY));
        }


        int connect() override
        {
            return 0;
        }

        bool sendRequest(scan_messages::ClientScanRequestPtr request) override
        {
            m_paths.emplace_back(request->getPath());
            //            PRINT("Scanning " << p);
            return true;
        }

        bool receiveResponse(scan_messages::ScanResponse& response) override
        {
            if (m_withDetections)
            {
                response.addDetection(m_paths.back(), "threatName","sha256");
            }
            if (m_withErrors)
            {
                response.setErrorMsg("Scan Error");
            }
            return true;
        }

        int socketFd() override
        {
            return m_socketFd.fd();
        }

        bool m_withDetections;
        bool m_withErrors;
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

        bool sendRequest(scan_messages::ClientScanRequestPtr) override
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

    class ExceptionThrowingTestSocket : public unixsocket::IScanningClientSocket
    {
    public:
        explicit ExceptionThrowingTestSocket(const bool scanInterrupted=true)
            : m_scanInterrupted(scanInterrupted)
        {
        }

        int connect() override
        {
            // real connect() method never throws.
            return 0;
        }

        bool sendRequest(scan_messages::ClientScanRequestPtr) override
        {
            if (m_scanInterrupted)
            {
                throw ScanInterruptedException("Scanner received stop notification");
            }
            else
            {
                throw sophos_on_access_process::onaccessimpl::ClientSocketException("Failed to send scan request");
            }
        }

        bool receiveResponse(scan_messages::ScanResponse&) override
        {
            if (m_scanInterrupted)
            {
                throw ScanInterruptedException("Scanner received stop notification");
            }
            else
            {
                throw sophos_on_access_process::onaccessimpl::ClientSocketException("Failed to receive scan response");
            }
        }

        int socketFd() override
        {
            return -1;
        }

        bool m_scanInterrupted;
    };
}
