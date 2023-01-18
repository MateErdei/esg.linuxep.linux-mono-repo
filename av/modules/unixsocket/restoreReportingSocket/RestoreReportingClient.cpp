// Copyright 2022 Sophos Limited. All rights reserved.

#include "RestoreReportingClient.h"

#include "common/ApplicationPaths.h"
#include "common/StringUtils.h"
#include "unixsocket/Logger.h"

#include <capnp/serialize-packed.h>

#include <sstream>
#include <string>
#include <utility>

namespace unixsocket
{
    RestoreReportingClient::RestoreReportingClient(std::shared_ptr<common::StoppableSleeper> sleeper) :
        BaseClient{ Plugin::getRestoreReportSocketPath(), "Restore Reporting Client", BaseClient::DEFAULT_SLEEP_TIME, std::move(sleeper) }
    {
        BaseClient::connectWithRetries();
    }

    void RestoreReportingClient::sendRestoreReport(const scan_messages::RestoreReport& restoreReport)
    {
        assert(m_socket_fd.valid());

        const std::string escapedPath = common::escapePathForLogging(restoreReport.path);
        auto successString = restoreReport.wasSuccessful ? "successful" : "unsuccessful";
        LOGINFO(m_name << " reports " << successString << " restoration of " << escapedPath);
        LOGDEBUG("Correlation ID: " << restoreReport.correlationId);

        ::capnp::writePackedMessageToFd(m_socket_fd.get(), *restoreReport.serialise());
    }
} // namespace unixsocket
