// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include "ScanningClientSocket.h"

#include "common/AbortScanException.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include "scan_messages/ClientScanRequest.h"

#include <ScanResponse.capnp.h>
#include <capnp/serialize.h>

#include <cassert>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace unixsocket
{
    ScanningClientSocket::ScanningClientSocket(std::string socket_path) :
        BaseClient(std::move(socket_path))
    {
    }

    int ScanningClientSocket::connect()
    {
        return attemptConnect();
    }

    bool ScanningClientSocket::sendRequest(const scan_messages::ClientScanRequestPtr request)
    {
        if (!m_socket_fd.valid())
        {
            return false;
        }
        std::string dataAsString = request->serialise();
        auto fd = request->getFd();

        try
        {
            if (!writeLengthAndBufferAndFd(m_socket_fd, dataAsString, fd))
            {
                return false;
            }
        }
        catch (environmentInterruption& e)
        {
            return false;
        }

        return true;
    }

    bool ScanningClientSocket::receiveResponse(scan_messages::ScanResponse& response)
    {
        if (!m_socket_fd.valid())
        {
            return false;
        }
        auto length = readLength(m_socket_fd);
        if (length < 0)
        {
            return false;
        }

        size_t buffer_size = 1 + length / sizeof(capnp::word);
        auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

        auto bytes_read = ::read(m_socket_fd, proto_buffer.begin(), length);

        if (bytes_read != length)
        {
            return false;
        }

        auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

        try
        {
            capnp::FlatArrayMessageReader messageInput(view);
            Sophos::ssplav::FileScanResponse::Reader responseReader =
                messageInput.getRoot<Sophos::ssplav::FileScanResponse>();

            response = scan_messages::ScanResponse(responseReader);
        }
        catch (const kj::Exception& ex)
        {
            if (ex.getType() == kj::Exception::Type::UNIMPLEMENTED)
            {
                // Fatal since this means we have a coding error that calls something unimplemented in kj.
                LOGFATAL(
                    "Terminated ScanningClientSocket with serialisation unimplemented exception: "
                    << ex.getDescription().cStr());
            }
            else
            {
                LOGERROR(
                    "Terminated ScanningClientSocket with serialisation exception: " << ex.getDescription().cStr());
            }

            std::stringstream errorMsg;
            errorMsg << "Malformed response from Sophos Threat Detector (" << ex.getDescription().cStr() << ")";
            throw common::AbortScanException(errorMsg.str());
        }
        catch (const std::exception& ex)
        {
            LOGFATAL("Caught non-kj::Exception attempting to parse ScanResponse: " << ex.what());
            std::stringstream errorMsg;
            errorMsg << "Malformed response from Sophos Threat Detector (" << ex.what() << ")";
            throw common::AbortScanException(errorMsg.str());
        }

        return true;
    }

    int ScanningClientSocket::socketFd()
    {
        return m_socket_fd;
    }
}