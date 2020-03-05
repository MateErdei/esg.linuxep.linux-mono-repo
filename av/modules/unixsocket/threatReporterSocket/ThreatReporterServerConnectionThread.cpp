/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterServerConnectionThread.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "ThreatDetected.capnp.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "datatypes/Print.h"
#include <capnp/serialize.h>

#include <stdexcept>
#include <iostream>
#include <utility>

#include <sys/socket.h>
#include <unistd.h>
#include <unixsocket/StringUtils.h>

using namespace unixsocket;

ThreatReporterServerConnectionThread::ThreatReporterServerConnectionThread(int fd,
                                                                           std::shared_ptr<IMessageCallback> callback)
        : m_fd(fd)
        , m_callback(std::move(callback))
{
}

static inline bool fd_isset(int fd, fd_set* fds)
{
    assert(fd >= 0);
    return FD_ISSET(static_cast<unsigned>(fd), fds); // NOLINT
}

static inline void internal_fd_set(int fd, fd_set* fds)
{
    assert(fd >= 0);
    FD_SET(static_cast<unsigned>(fd), fds); // NOLINT
}

static int addFD(fd_set* fds, int fd, int currentMax)
{
    internal_fd_set(fd, fds);
    return std::max(fd, currentMax);
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

void ThreatReporterServerConnectionThread::run()
{
    announceThreadStarted();

    datatypes::AutoFd socket_fd(std::move(m_fd));
    PRINT("Got connection " << socket_fd.fd());
    LOGDEBUG("Got connection " << socket_fd.fd());
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    int exitFD = m_notifyPipe.readFd();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = addFD(&readFDs, exitFD, max);
    max = addFD(&readFDs, socket_fd, max);

    while (true)
    {
        fd_set tempRead = readFDs;

        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);

        if (activity < 0)
        {
            LOGERROR("Socket failed: " << errno);
            break;
        }

        if (fd_isset(exitFD, &tempRead))
        {
            LOGINFO("Closing scanning socket thread");
            break;
        }

        if (fd_isset(socket_fd, &tempRead))
        {

            // read length
            int32_t length = unixsocket::readLength(socket_fd);
            if (length < 0)
            {
                PRINT("Aborting connection: failed to read length");
                return;
            }

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

            m_callback->processMessage(generateThreatDetectedXml(detectionReader));

            // TO DO: HANDLE ANY SOCKET ERRORS
        }
    }
}
