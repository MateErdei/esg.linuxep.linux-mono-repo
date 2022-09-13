// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ClientSocketWrapper.h"
#include "Logger.h"
#include "ReconnectSettings.h"

#include "common/AbortScanException.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"

#include <thread>

#include <poll.h>

namespace sophos_on_access_process::onaccessimpl
{
    ClientSocketWrapper::ClientSocketWrapper(
        unixsocket::IScanningClientSocket& socket,
        Common::Threads::NotifyPipe& notifyPipe,
        const struct timespec& retryInterval)
        : m_socket(socket)
        , m_notifyPipe(notifyPipe)
        , m_retryInterval(retryInterval)
    {
        ClientSocketWrapper::connect();
    }

    void ClientSocketWrapper::connect()
    {
        int count = 0;
        int ret = m_socket.connect();

        while (ret != 0)
        {
            if (++count >= MAX_CONN_RETRIES)
            {
                LOGDEBUG("Reached total maximum number of connection attempts.");
                return;
            }

            LOGDEBUG("Failed to connect to Sophos Threat Detector - retrying after sleep");
            interruptableSleep();

            ret = m_socket.connect();
        }

        m_reconnectAttempts = 0;
    }

    scan_messages::ScanResponse ClientSocketWrapper::scan(const scan_messages::ClientScanRequestPtr request)
    {
        bool retryErrorLogged = false;
        for (int attempt = 0; attempt < MAX_SCAN_RETRIES; attempt++)
        {
            if (m_reconnectAttempts >= TOTAL_MAX_RECONNECTS)
            {
                throw common::AbortScanException("Reached total maximum number of reconnection attempts. Aborting scan.");
            }

            checkIfScanAborted();

            try
            {
                scan_messages::ScanResponse response = attemptScan(request);
                if (m_reconnectAttempts > 0)
                {
                    checkIfScanAborted();
                    LOGINFO("Reconnected to Sophos Threat Detector after " << m_reconnectAttempts << " attempts");
                    m_reconnectAttempts = 0;
                }
                return response;
            }
            catch (const ClientSocketException& e)
            {
                if (!retryErrorLogged) // NOLINT
                {
                    checkIfScanAborted();
                    LOGWARN(e.what() << " - retrying after sleep");
                }
                interruptableSleep();
                if (!m_socket.connect())
                {
                    if (!retryErrorLogged)
                    {
                        LOGWARN("Failed to reconnect to Sophos Threat Detector - retrying...");
                    }
                }
                retryErrorLogged = true;
                m_reconnectAttempts++;
            }
        }

        scan_messages::ScanResponse response;
        std::stringstream errorMsg;
        std::string escapedPath(common::escapePathForLogging(request->getPath(), true));

        errorMsg << "Failed to scan file: " << escapedPath << " after " << MAX_SCAN_RETRIES << " retries";
        response.setErrorMsg(errorMsg.str());
        return response;
    }

    scan_messages::ScanResponse ClientSocketWrapper::attemptScan(scan_messages::ClientScanRequestPtr request)
    {
        if (!m_socket.sendRequest(request))
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

    void ClientSocketWrapper::interruptableSleep()
    {
        struct pollfd fds[] {
            { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        };

        while (true)
        {
            auto ret = ::ppoll(fds, std::size(fds), &m_retryInterval, nullptr);

            if (ret < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
                throw ClientSocketException("Error while sleeping");
            }

            else if (ret == 0)
            {
                // timeout
                return;
            }

            else // if (ret > 0)
            {
                checkIfScanAborted();
            }
        }
    }
}
