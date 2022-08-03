/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
        m_socketPath(std::move(socket_path))
    {
    }

    int ScanningClientSocket::connect()
    {
        m_socket_fd.reset(socket(AF_UNIX, SOCK_STREAM, 0));
        assert(m_socket_fd >= 0);

        struct sockaddr_un addr = {};
        addr.sun_family = AF_UNIX;
        ::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path));
        addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

        return ::connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    }

    bool ScanningClientSocket::sendRequest(
        datatypes::AutoFd& fd,
        const scan_messages::ClientScanRequest& request)
    {
        if (!m_socket_fd.valid())
        {
            return false;
        }
        std::string dataAsString = request.serialise();

        try
        {
            if (!writeLengthAndBuffer(m_socket_fd, dataAsString))
            {
                return false;
            }

            auto ret = send_fd(m_socket_fd, fd.get());
            if (ret < 0)
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
        catch (kj::Exception& ex)
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

        return true;
    }

    int ScanningClientSocket::socketFd()
    {
        return m_socket_fd;
    }
}