// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ClientSocketWrapper.h"
#include "Logger.h"

#include "common/SaferStrerror.h"
#include "common/StringUtils.h"

#include <poll.h>

namespace sophos_on_access_process::onaccessimpl
{
    ClientSocketWrapper::ClientSocketWrapper(unixsocket::IScanningClientSocket& socket, Common::Threads::NotifyPipe& notifyPipe)
        : m_socket(socket),
        m_notifyPipe(notifyPipe)
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
            { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
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

                LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
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
        if (m_notifyPipe.notified())
        {
            LOGDEBUG("Received stop notification");
            throw ScanInterruptedException("Scanner received stop notification");
        }
    }
}
