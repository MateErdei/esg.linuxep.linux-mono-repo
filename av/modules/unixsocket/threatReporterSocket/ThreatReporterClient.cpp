/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterClient.h"
#include "unixsocket/SocketUtils.h"

#include "datatypes/Print.h"
#include "scan_messages/ThreatDetected.h"
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

unixsocket::ThreatReporterClientSocket::ThreatReporterClientSocket(const std::string& socket_path)
{
    m_socket_fd.reset(socket(AF_UNIX, SOCK_STREAM, 0));
    assert(m_socket_fd >= 0);

    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    int ret = connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    if (ret != 0)
    {
        handle_error("Failed to connect to unix socket");
    }
}

void unixsocket::ThreatReporterClientSocket::sendThreatDetection(scan_messages::ThreatDetected detection)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = detection.serialise();

    if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
    {
        handle_error("Failed to write capn buffer to unix socket");
    }

    int32_t length = unixsocket::readLength(m_socket_fd);
    if (length < 0)
    {
        PRINT("Aborting connection: failed to read length");
        handle_error ("Failed to read length");
    }

    uint32_t buffer_size = 1 + length / sizeof(capnp::word);
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    ssize_t bytes_read = ::read(m_socket_fd, proto_buffer.begin(), length);
    if (bytes_read != length)
    {
        PRINT("Aborting connection: failed to read capn proto");
        handle_error ("Failed to read capn proto");
    }
}
