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

        using ThreatReporterServerSocketBase::ImplServerSocket;

    protected:
        TPtr makeThread(int fd, std::shared_ptr<IMessageCallback> callback) override
        {
            return std::make_unique<connection_thread_t>(fd, callback);
        }
    };
}

