/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

unixsocket::ThreatReporterClientSocket::ThreatReporterClientSocket(const std::string& socket_path)
{
    m_socket_fd.reset(socket(AF_UNIX, SOCK_STREAM, 0));
    assert(m_socket_fd >= 0);

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    int ret = connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    if (ret != 0)
    {
        handle_error("Failed to connect to unix socket");
    }
}

void unixsocket::ThreatReporterClientSocket::sendThreatDetection(const scan_messages::ThreatDetected& detection)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = detection.serialise();

    try
    {
        if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            handle_error("Failed to write capn buffer to unix socket");
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGERROR("Writing    to socket by Threat Report Client failed: " << e.what());
    }
}
