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
#include <utility>
#include <sstream>

#include <sys/socket.h>
#include <sys/un.h>

unixsocket::ProcessControllerClientSocket::ProcessControllerClientSocket(std::string socket_path, const struct timespec& sleepTime)
        : BaseClient(std::move(socket_path), sleepTime)
{
    connectWithRetries();
}

void unixsocket::ProcessControllerClientSocket::sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl)
{
    if (m_connectStatus == -1)
    {
        LOGWARN("Invalid connection status: message was not sent");
        return;
    }

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
