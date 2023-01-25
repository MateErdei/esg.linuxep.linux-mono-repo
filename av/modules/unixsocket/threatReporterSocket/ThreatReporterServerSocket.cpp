// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ThreatReporterServerSocket.h"

using namespace unixsocket;

ThreatReporterServerSocket::ThreatReporterServerSocket(
    const std::string& path,
    const mode_t mode,
    std::shared_ptr<IMessageCallback> threatReportCallback)
    : ThreatReporterServerSocketBase(path, "ThreatReporterServer", mode)
    , m_threatReportCallback(std::move(threatReportCallback))
{
}

ThreatReporterServerSocket::~ThreatReporterServerSocket()
{
    requestStop();
    join();
}