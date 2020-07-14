/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterServerSocket.h"

using namespace unixsocket;

ThreatReporterServerSocket::ThreatReporterServerSocket(const std::string& path, const mode_t mode,
        std::shared_ptr<IMessageCallback> callback)
    : ThreatReporterServerSocketBase(path, mode)
    , m_callback(std::move(callback))
{
}
