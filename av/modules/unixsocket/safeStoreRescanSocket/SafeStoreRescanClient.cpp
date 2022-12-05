// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreRescanClient.h"

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include <cassert>
#include <sstream>
#include <string>

unixsocket::SafeStoreRescanClient::SafeStoreRescanClient(
    std::string socket_path,
    const duration_t& sleepTime,
    IStoppableSleeperSharedPtr sleeper) :
    BaseClient(std::move(socket_path), sleepTime, std::move(sleeper))
{
    BaseClient::connectWithRetries("SafeStore Rescan");
}

void unixsocket::SafeStoreRescanClient::sendRescanRequest()
{
    LOGDEBUG("Sending rescan request to SafeStore");
    assert(m_socket_fd.valid());
    std::string dataAsString = "1";

    try
    {
        if (!writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            std::stringstream errMsg;
            errMsg << "Failed to write rescan request to socket [" << errno << "]";
            throw std::runtime_error(errMsg.str());
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGWARN("Failed to write to SafeStore Rescan socket. Exception caught: " << e.what());
    }
}
