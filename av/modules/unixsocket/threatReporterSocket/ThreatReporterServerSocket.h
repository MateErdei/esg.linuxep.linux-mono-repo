// Copyright 2020-2023 Sophos Limited. All rights reserved.
/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "unixsocket/BaseServerSocket.h"
#include "ThreatReporterServerConnectionThread.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

namespace unixsocket
{
    using ThreatReporterServerSocketBase = ImplServerSocket<ThreatReporterServerConnectionThread>;

    class ThreatReporterServerSocket : public ThreatReporterServerSocketBase
    {
    public:
        ThreatReporterServerSocket(
            const std::string& path,
            const mode_t mode,
            std::shared_ptr<IMessageCallback> threatReportCallback);
            ~ThreatReporterServerSocket() override;
    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            auto sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
            return std::make_unique<ThreatReporterServerConnectionThread>(fd, m_threatReportCallback, sysCalls);
        }

        void logMaxConnectionsError() override
        {
            logError("Refusing connection: Threat Reporter has reached the maximum allowed concurrent reports");
        }
    private:

        std::shared_ptr<IMessageCallback> m_threatReportCallback;
    };
}

