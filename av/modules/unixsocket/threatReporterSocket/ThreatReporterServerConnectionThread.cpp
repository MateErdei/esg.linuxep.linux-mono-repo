/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterServerConnectionThread.h"

#include "../Logger.h"

#include "datatypes/Print.h"
#include "unixsocket/SocketUtils.h"
#include <unixsocket/StringUtils.h>

#include "ThreatDetected.capnp.h"

#include <capnp/serialize.h>

#include <stdexcept>
#include <iostream>
#include <utility>

#include <sys/socket.h>
#include <unistd.h>
#include <scan_messages/ServerThreatDetected.h>

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
static scan_messages::ServerThreatDetected parseDetection(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    if (!requestReader.hasFilePath())
    {
        LOGERROR("parseDetection: report ( size=" << bytes_read <<") doesn't have file path!");
    }
    return scan_messages::ServerThreatDetected(requestReader);
}

void ThreatReporterServerConnectionThread::run()
{
    announceThreadStarted();

    datatypes::AutoFd socket_fd(std::move(m_fd));
    LOGDEBUG("Got connection " << socket_fd.fd());
    uint32_t buffer_size = 512;
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
                LOGERROR("Aborting connection: failed to read length");
                return;
            }

            if (length == 0)
            {
                LOGERROR("Ignoring length of zero");
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
                LOGERROR("Aborting connection: failed to read capn proto");
                return;
            }

            LOGDEBUG("Read capn of " << bytes_read);
            auto detectionReader = parseDetection(proto_buffer, bytes_read);

            if (!detectionReader.hasFilePath())
            {
                LOGERROR("Detection report ( size=" << bytes_read <<") doesn't have file path!");
            }
            else if (detectionReader.getFilePath() == "")
            {
                LOGERROR("Detection report has empty file path!");
            }
            m_callback->processMessage(generateThreatDetectedXml(detectionReader));

            // TO DO: HANDLE ANY SOCKET ERRORS
        }
    }
}
