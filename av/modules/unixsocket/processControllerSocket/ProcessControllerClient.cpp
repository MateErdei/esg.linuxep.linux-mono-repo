// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ProcessControllerClient.h"

#include "common/SaferStrerror.h"

#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include "unixsocket/UnixSocketException.h"

#include <string>
#include <cassert>
#include <utility>
#include <sstream>

unixsocket::ProcessControllerClientSocket::ProcessControllerClientSocket(std::string socket_path, const duration_t& sleepTime)
        : BaseClient(std::move(socket_path), "ProcessControlClient", sleepTime)
{
    connectWithRetries();
}

unixsocket::ProcessControllerClientSocket::ProcessControllerClientSocket(
    std::string socket_path,
    unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper,
    int max_retries,
    const unixsocket::BaseClient::duration_t& sleepTime)
    : BaseClient(std::move(socket_path), "ProcessControlClient", sleepTime, std::move(sleeper))
{
    if (max_retries >= 0)
    {
        connectWithRetries(max_retries);
    }
}

void unixsocket::ProcessControllerClientSocket::sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl)
{
    if (m_connectStatus == -1)
    {
        LOGWARN(m_name << " has invalid connection status: message was not sent");
        return;
    }

    assert(m_socket_fd >= 0);
    std::string dataAsString = processControl.serialise();

    try
    {
        if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            std::stringstream errMsg;
            errMsg << m_name << " failed to write Process Control Request to socket [" << common::safer_strerror(errno) << "]";
            throw unixsocket::UnixSocketException(LOCATION, errMsg.str());
        }
    }
    catch (unixsocket::EnvironmentInterruption& e)
    {
        LOGWARN(m_name << "failed to write Process Control Request to socket "<< m_socketPath << ". Exception caught: " << e.what());
    }
}