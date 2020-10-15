/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningClientSocket.h"

#include "ReconnectScannerException.h"

#include "common/AbortScanException.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include "scan_messages/ClientScanRequest.h"
#include <ScanResponse.capnp.h>

#include <capnp/serialize.h>

#include <string>
#include <sstream>
#include <cassert>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

#define TOTAL_MAX_RECONNECTS 100
#define MAX_CONN_RETRIES 5
#define MAX_SCAN_RETRIES 10

unixsocket::ScanningClientSocket::ScanningClientSocket(std::string socket_path, const struct timespec& sleepTime)
    : m_reconnectAttempts(0)
    , m_socketPath(std::move(socket_path))
    , m_sleepTime(sleepTime)
{
    connect();
}


void unixsocket::ScanningClientSocket::connect()
{
    int count = 0;
    int ret = attemptConnect();

    while (ret != 0)
    {
        if (++count >= MAX_CONN_RETRIES)
        {
            LOGDEBUG("Reached total maximum number of connection attempts.");
            return;
        }

        LOGDEBUG("Failed to connect to Sophos Threat Detector - retrying after sleep");
        nanosleep(&m_sleepTime, nullptr);

        ret = attemptConnect();
    }

    assert(ret == 0);
    m_reconnectAttempts = 0;
}

int unixsocket::ScanningClientSocket::attemptConnect()
{
    m_socket_fd.reset(socket(AF_UNIX, SOCK_STREAM, 0));
    assert(m_socket_fd >= 0);

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    return ::connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
}



scan_messages::ScanResponse
unixsocket::ScanningClientSocket::scan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request)
{
    for (int attempt=0; attempt < MAX_SCAN_RETRIES; attempt++)
    {
        if (m_reconnectAttempts >= TOTAL_MAX_RECONNECTS)
        {
            throw AbortScanException("Reached total maximum number of reconnection attempts. Aborting scan.");
        }

        try
        {
            scan_messages::ScanResponse response = attemptScan(fd, request);
            if (m_reconnectAttempts > 0)
            {
                m_reconnectAttempts = 0;
                LOGINFO("Reconnected to Sophos Threat Detector");
            }
            return response;
        }
        catch (const ReconnectScannerException& e)
        {
            LOGERROR(e.what() << " - retrying after sleep");
            nanosleep(&m_sleepTime, nullptr);
            if (!attemptConnect())
            {
                LOGWARN("Failed to reconnect to Sophos Threat Detector - retrying...");
            }
            m_reconnectAttempts++;
        }
    }

    scan_messages::ScanResponse response;
    std::stringstream errorMsg;

    errorMsg << "Failed to scan file: " << request.getPath() << " after " << MAX_SCAN_RETRIES << " retries";
    response.setErrorMsg(errorMsg.str());
    return response;
}

scan_messages::ScanResponse
unixsocket::ScanningClientSocket::attemptScan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = request.serialise();

    try
    {
        if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            throw ReconnectScannerException("Failed to send scan request to Sophos Threat Detector");
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        std::stringstream errorMsg;
        errorMsg << "Failed to send scan request to Sophos Threat Detector (" << e.what() << ")";
        throw ReconnectScannerException(errorMsg.str());
    }

    int ret = unixsocket::send_fd(m_socket_fd, fd.get());
    if (ret < 0)
    {
        std::stringstream errorMsg;
        errorMsg << "Failed to send message: " << std::strerror(errno);
        throw AbortScanException(errorMsg.str());
    }

    int32_t length = unixsocket::readLength(m_socket_fd);
    if (length < 0)
    {
        throw ReconnectScannerException("Failed to read length");
    }

    uint32_t buffer_size = 1 + length / sizeof(capnp::word);
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    ssize_t bytes_read = ::read(m_socket_fd, proto_buffer.begin(), length);
    if (bytes_read != length)
    {
        throw ReconnectScannerException("Failed to read message from Sophos Threat Detector");
    }

    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::FileScanResponse::Reader responseReader =
            messageInput.getRoot<Sophos::ssplav::FileScanResponse>();

    scan_messages::ScanResponse response(responseReader);

    return response;
}