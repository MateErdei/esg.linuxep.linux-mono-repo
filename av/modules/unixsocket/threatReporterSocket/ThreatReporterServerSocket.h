/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

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
            ~ThreatReporterServerSocket();
    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<ThreatReporterServerConnectionThread>(fd, m_threatReportCallback);
        }
    private:

        std::shared_ptr<IMessageCallback> m_threatReportCallback;
    };
}

