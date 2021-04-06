/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterServerSocket.h"

using namespace unixsocket;

ThreatReporterServerSocket::ThreatReporterServerSocket(
    const std::string& path,
    const mode_t mode,
    std::shared_ptr<IMessageCallback> threatReportCallback,
    std::shared_ptr<IMessageCallback> threatEventPublisherCallback)
    : ThreatReporterServerSocketBase(path, mode)
    , m_threatReportCallback(std::move(threatReportCallback))
    , m_threatEventPublisherCallback(std::move(threatEventPublisherCallback))
{
}
