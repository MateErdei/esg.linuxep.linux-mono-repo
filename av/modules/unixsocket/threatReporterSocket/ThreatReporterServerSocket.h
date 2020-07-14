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
        ThreatReporterServerSocket(const std::string& path, const mode_t mode, std::shared_ptr<IMessageCallback> callback);

    protected:
        TPtr makeThread(int fd) override
        {
            return std::make_unique<ThreatReporterServerConnectionThread>(fd, m_callback);
        }
    private:

        std::shared_ptr<IMessageCallback> m_callback;
    };
}

