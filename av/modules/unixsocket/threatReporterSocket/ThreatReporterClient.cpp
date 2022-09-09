// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ThreatReporterClient.h"

#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include "scan_messages/ThreatDetected.h"
#include <ScanResponse.capnp.h>

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <sys/socket.h>
#include <sys/un.h>
#include <sstream>

unixsocket::ThreatReporterClientSocket::ThreatReporterClientSocket(std::string socket_path, const struct timespec& sleepTime)
    : BaseClient(std::move(socket_path), sleepTime)
{
    connectWithRetries("Threat reporter");
}

void unixsocket::ThreatReporterClientSocket::sendThreatDetection(const scan_messages::ThreatDetected& detection)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = detection.serialise();

    try
    {
        if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
                std::stringstream errMsg;
                errMsg << "Failed to write Threat Report to socket [" << errno << "]";
                throw std::runtime_error(errMsg.str());
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGERROR("Failed to write Threat Report Client to socket. Exception caught: " << e.what());
    }
}

