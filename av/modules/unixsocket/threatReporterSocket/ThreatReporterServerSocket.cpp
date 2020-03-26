/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterServerSocket.h"

using namespace unixsocket;

ThreatReporterServerSocket::ThreatReporterServerSocket(const std::string& path,
        std::shared_ptr<IMessageCallback> callback)
    : ThreatReporterServerSocketBase(path)
    , m_callback(std::move(callback))
{
}
