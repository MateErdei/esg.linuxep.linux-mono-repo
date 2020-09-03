/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningClientSocket.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include "scan_messages/ClientScanRequest.h"
#include <ScanResponse.capnp.h>

#include <capnp/serialize.h>

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

#define MAX_CONN_RETRIES 10

unixsocket::ScanningClientSocket::ScanningClientSocket(const std::string& socket_path)
{
    m_socket_fd.reset(socket(AF_UNIX, SOCK_STREAM, 0));
    assert(m_socket_fd >= 0);

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    int ret = connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    int count = 0;
    while (ret != 0)
    {
        if (++count >= MAX_CONN_RETRIES)
        {
            handle_error("Failed to connect to Sophos Threat Detector, aborting the scan.");
        }

        LOGDEBUG("Failed to connect to Sophos Threat Detector - retrying in 1 second");

        sleep(1);
        ret = connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    }
}

static
void send_fd(int socket, int fd)  // send fd by socket
{
    struct msghdr msg = {};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(fd))];
    char dup[256];
    memset(buf, '\0', sizeof(buf));
    struct iovec io = { .iov_base = &dup, .iov_len = sizeof(dup) };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

    memcpy (CMSG_DATA(cmsg), &fd, sizeof (fd));

    if (sendmsg (socket, &msg, 0) < 0)
    {
        handle_error ("Failed to send message");
    }
}

scan_messages::ScanResponse
unixsocket::ScanningClientSocket::scan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = request.serialise();

    try
    {
        if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            handle_error("Failed to write capn buffer to unix socket");
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGERROR("Failed to write capnp buffer to unix socket, no scan request sent: " << e.what());
        handle_error("Scan terminated failed to initiate scan");
    }

    send_fd(m_socket_fd, fd.get());

    int32_t length = unixsocket::readLength(m_socket_fd);
    if (length < 0)
    {
        LOGERROR("Aborting connection: failed to read length");
        handle_error ("Failed to read length");
    }



    uint32_t buffer_size = 1 + length / sizeof(capnp::word);
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    ssize_t bytes_read = ::read(m_socket_fd, proto_buffer.begin(), length);
    if (bytes_read != length)
    {
        LOGERROR("Aborting connection: failed to read capn proto");
        handle_error ("Failed to read capn proto");
    }



    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::FileScanResponse::Reader responseReader =
            messageInput.getRoot<Sophos::ssplav::FileScanResponse>();


    scan_messages::ScanResponse response;

    response.setClean(responseReader.getClean());
    if (responseReader.hasThreatName())
    {
        response.setThreatName(responseReader.getThreatName());
    }

    return response;
}
