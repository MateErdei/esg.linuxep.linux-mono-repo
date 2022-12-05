// Copyright 2022 Sophos Limited. All rights reserved.

#include "RestoreReportingServer.h"

#include "../Logger.h"
#include "common/ApplicationPaths.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"

#include <RestoreReport.capnp.h>

#include <capnp/serialize-packed.h>

namespace
{
    enum class PollStatus
    {
        readyToRead,
        skip,
        exit
    };

    PollStatus poll(int fd, int notifyPipeFd)
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
                    LOGDEBUG("Ignoring EINTR from ppoll");
                    continue;
                }

                throw std::runtime_error("ppoll error: " + common::safer_strerror(errno));
            }

            assert(ret > 0); // 0 is not possible since ppoll can't time out

            if (pollFds[1].revents != 0)
            {
                if ((pollFds[1].revents & POLLIN) != 0)
                {
                    LOGDEBUG("Restore Reporting Server got notified by NotifyPipe");
                }
                else if ((pollFds[1].revents & POLLERR) != 0)
                {
                    LOGERROR("Restore Reporting Server got error on NotifyPipe fd");
                }
                else
                {
                    LOGWARN("Restore Reporting Server NotifyPipe closed before notifying");
                }

                return PollStatus::exit;
            }

            if ((pollFds[0].revents & (POLLHUP | POLLNVAL)) != 0)
            {
                LOGDEBUG("Restore Reporting Server connection fd closed");
            }

            if ((pollFds[0].revents & POLLERR) != 0)
            {
                LOGDEBUG("Restore Reporting Server got error on connection fd");
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
}

namespace unixsocket
{
    RestoreReportingServer::RestoreReportingServer(const IRestoreReportProcessor& restoreReportProcessor) :
        BaseServerSocket{ Plugin::getRestoreReportSocketPath(), 0600 },
        m_restoreReportProcessor{ restoreReportProcessor }
    {
        m_socketName = "Restore Reporting Server";
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
            LOGDEBUG("Restore Reporting Server got connection with fd " << connectionFd.get());

            auto pollStatus = poll(connectionFd.get(), m_notifyPipe.readFd());
            switch (pollStatus)
            {
                case PollStatus::readyToRead:
                    break;
                case PollStatus::skip:
                    LOGDEBUG("Restore Reporting Server awaiting new connection");
                    return false;
                case PollStatus::exit:
                    LOGDEBUG("Closing Restore Reporting Server");
                    return true;
            }

            LOGDEBUG("Restore Reporting Server ready to receive data");

            ::capnp::PackedFdMessageReader message{ connectionFd.get() };
            auto restoreReportReader = message.getRoot<Sophos::ssplav::RestoreReport>();

            LOGDEBUG("Restore Reporting Server received a report");

            const scan_messages::RestoreReport restoreReport{ restoreReportReader };

            restoreReport.validate();

            LOGDEBUG(
                "Path: " << common::escapePathForLogging(restoreReport.path)
                         << ", correlation ID: " << restoreReport.correlationId << ", "
                         << (restoreReport.wasSuccessful ? "successful" : "unsuccessful"));

            m_restoreReportProcessor.processRestoreReport(restoreReport);
        }
        catch (const std::exception& e)
        {
            LOGERROR("Restore Reporting Server failed to receive a report: " << e.what());
        }

        LOGDEBUG("Restore Reporting Server awaiting new connection");
        return false;
    }

    // We don't spawn connection threads, so do nothing
    void RestoreReportingServer::killThreads() {}
} // namespace unixsocket