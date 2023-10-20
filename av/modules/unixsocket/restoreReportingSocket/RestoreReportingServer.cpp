// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "RestoreReportingServer.h"

#include "unixsocket/Logger.h"
#include "unixsocket/UnixSocketException.h"

#include "common/ApplicationPaths.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include "scan_messages/RestoreReport.capnp.h"

#include <capnp/serialize-packed.h>

namespace unixsocket
{
    RestoreReportingServer::RestoreReportingServer(const IRestoreReportProcessor& restoreReportProcessor) :
        BaseServerSocket{ Plugin::getRestoreReportSocketPath(), "RestoreReportingServer", 0600 },
        m_restoreReportProcessor{ restoreReportProcessor }
    {
    }

    RestoreReportingServer::~RestoreReportingServer()
    {
        requestStop();
        join();
    }

    bool RestoreReportingServer::handleConnection(datatypes::AutoFd& connectionFd)
    {
        try
        {
            LOGDEBUG(m_socketName << " got connection with fd " << connectionFd.get());

            auto pollStatus = poll(connectionFd.get(), m_notifyPipe.readFd());
            switch (pollStatus)
            {
                case PollStatus::readyToRead:
                    break;
                case PollStatus::skip:
                    LOGDEBUG(m_socketName << " awaiting new connection");
                    return false;
                case PollStatus::exit:
                    LOGDEBUG("Closing " << m_socketName);
                    return true;
            }

            LOGDEBUG(m_socketName << " ready to receive data");

            ::capnp::PackedFdMessageReader message{ connectionFd.get() };
            auto restoreReportReader = message.getRoot<Sophos::ssplav::RestoreReport>();

            LOGDEBUG(m_socketName << " received a report");

            const scan_messages::RestoreReport restoreReport{ restoreReportReader };

            restoreReport.validate();

            auto successStr = restoreReport.wasSuccessful ? "successful" : "unsuccessful";
            LOGDEBUG(m_socketName << ": Path: " << common::escapePathForLogging(restoreReport.path)
                         << ", correlation ID: " << restoreReport.correlationId << ", "
                         << successStr);

            m_restoreReportProcessor.processRestoreReport(restoreReport);
        }
        catch (const std::exception& e)
        {
            LOGERROR(m_socketName << " failed to receive a report: " << e.what());
        }

        LOGDEBUG(m_socketName << " awaiting new connection");
        return false;
    }

    RestoreReportingServer::PollStatus RestoreReportingServer::poll(int fd, int notifyPipeFd)
    {
        struct pollfd pollFds[] = {
            { .fd = fd, .events = POLLIN, .revents = 0 },
            { .fd = notifyPipeFd, .events = POLLIN, .revents = 0 },
        };

        while (true)
        {
            // wait for an activity on one of the fds, timeout is NULL, so wait indefinitely
            auto ret = ::ppoll(pollFds, std::size(pollFds), nullptr, nullptr);
            if (ret < 0)
            {
                if (errno == EINTR)
                {
                    LOGDEBUG(m_socketName << " ignoring EINTR from ppoll");
                    continue;
                }

                throw unixsocket::UnixSocketException(LOCATION, m_socketName + " ppoll error: " + common::safer_strerror(errno));
            }

            assert(ret > 0); // 0 is not possible since ppoll can't time out

            if (pollFds[1].revents != 0)
            {
                if ((pollFds[1].revents & POLLIN) != 0)
                {
                    LOGDEBUG(m_socketName << " got notified by NotifyPipe");
                }
                else if ((pollFds[1].revents & POLLERR) != 0)
                {
                    LOGERROR(m_socketName << " got error on NotifyPipe fd");
                }
                else
                {
                    LOGWARN(m_socketName << " NotifyPipe closed before notifying");
                }

                return PollStatus::exit;
            }

            if ((pollFds[0].revents & (POLLHUP | POLLNVAL)) != 0)
            {
                LOGDEBUG(m_socketName << " connection fd closed");
            }

            if ((pollFds[0].revents & POLLERR) != 0)
            {
                LOGDEBUG(m_socketName << " got error on connection fd");
                return PollStatus::skip;
            }
            else if ((pollFds[0].revents & POLLIN) != 0)
            {
                return PollStatus::readyToRead;
            }
            else
            {
                return PollStatus::skip;
            }
        }
    }

    // We don't spawn connection threads, so do nothing
    void RestoreReportingServer::killThreads() {}
} // namespace unixsocket