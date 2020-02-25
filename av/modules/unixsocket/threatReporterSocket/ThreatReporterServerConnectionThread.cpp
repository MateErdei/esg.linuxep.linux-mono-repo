/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterServerConnectionThread.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "ThreatDetected.capnp.h"

#include "datatypes/Print.h"
#include <capnp/serialize.h>

#include <stdexcept>
#include <iostream>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>

unixsocket::ThreatReporterServerConnectionThread::ThreatReporterServerConnectionThread(int fd)
        : m_finished(false), m_fd(fd)
{
}

bool unixsocket::ThreatReporterServerConnectionThread::finished()
{
    return m_finished;
}

void unixsocket::ThreatReporterServerConnectionThread::start()
{

}

void unixsocket::ThreatReporterServerConnectionThread::notifyTerminate()
{

}
//
//static void throwOnError(int ret, const std::string& message)
//{
//    if (ret == 0)
//    {
//        return;
//    }
//    perror(message.c_str());
//    throw std::runtime_error(message);
//}

/**
 * Parse a detection.
 *
 * Placed in a function to ensure the MessageReader and view are deleted before the buffer might become invalid.
 *
 * @param proto_buffer
 * @param bytes_read
 * @return
 */
static  Sophos::ssplav::ThreatDetected::Reader parseDetection(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    return requestReader;
}

void unixsocket::ThreatReporterServerConnectionThread::run()
{
    datatypes::AutoFd socket_fd(std::move(m_fd));
    PRINT("Got connection " << socket_fd.fd());
    LOGDEBUG("Got connection " << socket_fd.fd());
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    while (true)
    {
        // read length
        int32_t length = unixsocket::readLength(socket_fd);
        if (length < 0)
        {
            PRINT("Aborting connection: failed to read length");
            return;
        }

        PRINT("Read a length of " << length);
        if (length == 0)
        {
            PRINT("Ignoring length of zero");
            continue;
        }

        // read capn proto
        if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
        {
            buffer_size = 1 + length / sizeof(capnp::word);
            proto_buffer = kj::heapArray<capnp::word>(buffer_size);
        }

        ssize_t bytes_read = ::read(socket_fd, proto_buffer.begin(), length);
        if (bytes_read != length)
        {
            PRINT("Aborting connection: failed to read capn proto");
            return;
        }

        PRINT("Read capn of " << bytes_read);
        Sophos::ssplav::ThreatDetected::Reader detectionReader = parseDetection(proto_buffer, bytes_read);
        // TO DO WRITE SERVER FUNCTIONALITY
        // call an AVP manager method/callback that generates the XML
        // pass the arguments needed to generate the XML in the callback/method
        // so that the AVP wont have to deal with capnp objects

        // do that so I can build
        detectionReader.getUserID();
        // TO DO: HANDLE ANY SOCKET ERRORS
    }
}