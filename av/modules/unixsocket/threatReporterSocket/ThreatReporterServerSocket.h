/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/SystemCallWrapper.h"
#include "unixsocket/BaseServerSocket.h"
#include "ThreatReporterServerConnectionThread.h"

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
            auto sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
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

