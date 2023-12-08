// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SafeStoreRescanClient.h"

#include "common/SaferStrerror.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/UnixSocketException.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <cassert>
#include <sstream>
#include <string>

unixsocket::SafeStoreRescanClient::SafeStoreRescanClient(
    std::string socket_path,
    const duration_t& sleepTime,
    IStoppableSleeperSharedPtr sleeper) :
    BaseClient(std::move(socket_path), "SafeStoreRescanClient", sleepTime, std::move(sleeper))
{
    BaseClient::connectWithRetries();
}

void unixsocket::SafeStoreRescanClient::sendRescanRequest()
{
    LOGDEBUG(m_name + " sending rescan request to SafeStore");
    assert(m_socket_fd.valid());
    std::string dataAsString = "1";

    try
    {
        Common::SystemCallWrapper::SystemCallWrapper systemCallWrapper;
        if (!writeLengthAndBuffer(systemCallWrapper, m_socket_fd, dataAsString))
        {
            std::stringstream errMsg;
            errMsg << m_name << " failed to write rescan request to socket [" << common::safer_strerror(errno) << "]";
            throw unixsocket::UnixSocketException(LOCATION, errMsg.str());
        }
    }
    catch (unixsocket::EnvironmentInterruption& e)
    {
        LOGWARN(m_name + " failed to write to socket. Exception caught: " << e.what());
    }
}
