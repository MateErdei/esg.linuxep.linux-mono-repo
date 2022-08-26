// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ClientSocketWrapper.h"
#include "ReconnectSettings.h"
#include "ReconnectScannerException.h"
#include "Logger.h"

#include "common/AbortScanException.h"
#include "common/SaferStrerror.h"
#include "common/ScanInterruptedException.h"
#include "common/ScanManuallyInterruptedException.h"
#include <common/StringUtils.h>

#include <sstream>

#include <poll.h>


namespace avscanner::avscannerimpl
{
    ClientSocketWrapper::ClientSocketWrapper(unixsocket::IScanningClientSocket& socket, const struct timespec& sleepTime)
        : m_socket(socket),
        m_sigIntMonitor(common::signals::SigIntMonitor::getSigIntMonitor(false)),
        m_sigTermMonitor(common::signals::SigTermMonitor::getSigTermMonitor(false)),
        m_sigHupMonitor(common::signals::SigHupMonitor::getSigHupMonitor(false)),
        m_reconnectAttempts(0),
        m_sleepTime(sleepTime)
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
            interruptableSleep(&m_sleepTime);

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
            catch (const ReconnectScannerException& e)
            {
                if (!retryErrorLogged) // NOLINT
                {
                    checkIfScanAborted();
                    LOGWARN(e.what() << " - retrying after sleep");
                }
                interruptableSleep(&m_sleepTime);
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

    scan_messages::ScanResponse ClientSocketWrapper::attemptScan(const scan_messages::ClientScanRequestPtr& request)
    {
        auto autoFd = request->getAutoFd();
        if (!m_socket.sendRequest(request))
        {
            throw ReconnectScannerException("Failed to send scan request");
        }

        waitForResponse();

        scan_messages::ScanResponse response;
        if (!m_socket.receiveResponse(response))
        {
            throw ReconnectScannerException("Failed to receive scan response");
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

                LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
                throw ReconnectScannerException("Error while waiting for scan response");
            }

            else if (ret > 0)
            {
                checkIfScanAborted();
                break;
            }
        }
    }

    void ClientSocketWrapper::interruptableSleep(const timespec* duration)
    {
        struct pollfd fds[] {
            { .fd = m_sigHupMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
            { .fd = m_sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
            { .fd = m_sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        };

        while (true)
        {
            auto ret = ::ppoll(fds, std::size(fds), duration, nullptr);

            if (ret < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
                throw ReconnectScannerException("Error while sleeping");
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
