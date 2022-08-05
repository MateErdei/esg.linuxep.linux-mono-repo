/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ClientSocketWrapper.h"
#include "Logger.h"

#include "common/ScanManuallyInterruptedException.h"
#include "common/ScanInterruptedException.h"
#include <common/StringUtils.h>

#include <poll.h>

namespace sophos_on_access_process::onaccessimpl
{
    ClientSocketWrapper::ClientSocketWrapper(unixsocket::IScanningClientSocket& socket)
        : m_socket(socket),
        m_sigIntMonitor(common::SigIntMonitor::getSigIntMonitor()),
        m_sigTermMonitor(common::SigTermMonitor::getSigTermMonitor()),
        m_sigHupMonitor(common::SigHupMonitor::getSigHupMonitor())

    {
        ClientSocketWrapper::connect();
    }

    void ClientSocketWrapper::connect()
    {
        int ret = m_socket.connect();

        if (ret != 0)
        {
            throw ClientSocketException("Connect failed");
        }
    }

    scan_messages::ScanResponse ClientSocketWrapper::scan(
        datatypes::AutoFd& fd,
        const scan_messages::ClientScanRequest& request)
    {
        if (!m_socket.sendRequest(fd, request))
        {
            throw ClientSocketException("Failed to send scan request");
        }

        waitForResponse();

        scan_messages::ScanResponse response;
        if (!m_socket.receiveResponse(response))
        {
            throw ClientSocketException("Failed to receive scan response");
        }
        return response;
    }

    void ClientSocketWrapper::waitForResponse()
    {
        struct pollfd fds[] {
            { .fd = m_socket.socketFd(), .events = POLLIN, .revents = 0 },
            { .fd = m_sigHupMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
            { .fd = m_sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
            { .fd = m_sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        };

        while (true)
        {
            auto ret = ::ppoll(fds, std::size(fds), nullptr, nullptr);

            if (ret < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                LOGERROR("Error from ppoll: " << strerror(errno));
                throw ClientSocketException("Error while waiting for scan response");
            }

            else if (ret > 0)
            {
                checkIfScanAborted();
                break;
            }
        }
    }

    void ClientSocketWrapper::checkIfScanAborted()
    {
        if (m_sigIntMonitor->triggered())
        {
            LOGDEBUG("Received SIGINT");
            throw ScanManuallyInterruptedException("Scan manually aborted");
        }

        if (m_sigTermMonitor->triggered())
        {
            LOGDEBUG("Received SIGTERM");
            throw ScanInterruptedException("Scan aborted due to environment interruption");
        }

        if (m_sigHupMonitor->triggered())
        {
            LOGDEBUG("Received SIGHUP");
            throw ScanInterruptedException("Scan aborted due to environment interruption");
        }
    }
}
