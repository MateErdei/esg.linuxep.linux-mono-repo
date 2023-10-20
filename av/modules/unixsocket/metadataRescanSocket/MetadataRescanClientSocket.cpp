// Copyright 2020-2023 Sophos Limited. All rights reserved.

#define AUTO_FD_IMPLICIT_INT

#include "MetadataRescanClientSocket.h"

#include "common/AbortScanException.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include <unistd.h>

#include <capnp/serialize.h>

#include <sstream>

namespace unixsocket
{
    MetadataRescanClientSocket::MetadataRescanClientSocket(
        std::string socket_path,
        const duration_t& sleepTime,
        IStoppableSleeperSharedPtr sleeper) :
        BaseClient(std::move(socket_path), "MetadataRescanClient", sleepTime, std::move(sleeper))
    {
    }

    int MetadataRescanClientSocket::connect()
    {
        return attemptConnect();
    }

    bool MetadataRescanClientSocket::sendRequest(const scan_messages::MetadataRescan& metadataRescan)
    {
        if (!m_socket_fd.valid())
        {
            return false;
        }
        std::string dataAsString = metadataRescan.Serialise();

        try
        {
            if (!writeLengthAndBuffer(m_socket_fd, dataAsString))
            {
                return false;
            }
        }
        catch (const EnvironmentInterruption& e)
        {
            LOGDEBUG("Failed to send request at " << e.where_ << " errno=" << errno);
            return false;
        }

        return true;
    }

    bool MetadataRescanClientSocket::receiveResponse(scan_messages::MetadataRescanResponse& response)
    {
        if (!m_socket_fd.valid())
        {
            return false;
        }

        uint8_t responseByte;
        auto bytesRead = ::read(m_socket_fd.get(), &responseByte, sizeof(scan_messages::MetadataRescanResponse));
        if (bytesRead != sizeof(scan_messages::MetadataRescanResponse))
        {
            return false;
        }

        if (responseByte > 4)
        {
            LOGWARN("Invalid response");
            return false;
        }

        response = static_cast<scan_messages::MetadataRescanResponse>(responseByte);
        return true;
    }

    int MetadataRescanClientSocket::socketFd()
    {
        return m_socket_fd;
    }

    scan_messages::MetadataRescanResponse MetadataRescanClientSocket::rescan(
        const scan_messages::MetadataRescan& metadataRescan)
    {
        if (!isConnected())
        {
            connect();
        }

        if (!sendRequest(metadataRescan))
        {
            return scan_messages::MetadataRescanResponse::failed;
        }

        scan_messages::MetadataRescanResponse response;
        if (!receiveResponse(response))
        {
            return scan_messages::MetadataRescanResponse::failed;
        }

        return response;
    }
} // namespace unixsocket