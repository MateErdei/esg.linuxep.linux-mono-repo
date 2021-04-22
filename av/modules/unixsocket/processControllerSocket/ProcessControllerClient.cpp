/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessControllerClient.h"

#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include <ScanResponse.capnp.h>

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <sys/socket.h>
#include <sys/un.h>

unixsocket::ProcessControllerClientSocket::ProcessControllerClientSocket(const std::string& socket_path)
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
        throw std::runtime_error("Failed to connect to unix socket");
    }
}

void unixsocket::ProcessControllerClientSocket::sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = processControl.serialise();

    try
    {
        if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            std::stringstream errMsg;
            errMsg << "Failed to write Process Control Client to socket [" << errno << "]";
            throw std::runtime_error(errMsg.str());
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGERROR("Failed to write Process Control Request to socket. Exception caught: " << e.what());
    }
}
