// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ProcessControllerClient.h"

#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"

#include <string>
#include <cassert>
#include <utility>
#include <sstream>

unixsocket::ProcessControllerClientSocket::ProcessControllerClientSocket(std::string socket_path, const duration_t& sleepTime)
        : BaseClient(std::move(socket_path), "Process Controller", sleepTime)
{
    connectWithRetries();
}

unixsocket::ProcessControllerClientSocket::ProcessControllerClientSocket(
    std::string socket_path,
    unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper,
    int max_retries,
    const unixsocket::BaseClient::duration_t& sleepTime)
    : BaseClient(std::move(socket_path), sleepTime, std::move(sleeper))
{
    if (max_retries >= 0)
    {
        connectWithRetries(m_socketPath, max_retries);
    }
}

void unixsocket::ProcessControllerClientSocket::sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl)
{
    if (m_connectStatus == -1)
    {
        LOGWARN("Process Control has invalid connection status: message was not sent");
        return;
    }

    assert(m_socket_fd >= 0);
    std::string dataAsString = processControl.serialise();

    try
    {
        if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            std::stringstream errMsg;
            errMsg << "Failed to write Process Control Request to socket [" << errno << "]";
            throw std::runtime_error(errMsg.str());
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGWARN("Failed to write Process Control Request to socket "<< m_socketPath << ". Exception caught: " << e.what());
    }
}