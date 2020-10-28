/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServerConnectionThread.h"

#include "ScanRequest.capnp.h"

#include "datatypes/Print.h"
#include "unixsocket/SocketUtils.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <unixsocket/threatDetectorSocket/ScanningServerConnectionThread.h>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <utility>

#include <sys/socket.h>
#include <unistd.h>

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

FakeDetectionServer::FakeServerConnectionThread::FakeServerConnectionThread(
    datatypes::AutoFd socketFd,
    std::shared_ptr<std::vector<uint8_t>> Data)
    :  m_socketFd(std::move(socketFd))
    , m_Data(std::move(Data))
{
}

void FakeDetectionServer::FakeServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_socketFd));
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    int exitFD = m_notifyPipe.readFd();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = addFD(&readFDs, exitFD, max);
    max = addFD(&readFDs, socket_fd.get(), max);

    while (true)
    {
        fd_set tempRead = readFDs;

        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);

        if (activity < 0)
        {
            break;
        }
        // We don't set a timeout so something should have happened
        assert(activity != 0);

        if (fd_isset(exitFD, &tempRead))
        {
            break;
        }
        else // if(fd_isset(socket_fd, &tempRead))
        {
            // If shouldn't be required - we have no timeout, and only 2 FDs in the pselect.
            // exitFD will cause break
            // therefore "else" must be fd_isset(socket_fd, &tempRead)
            assert(fd_isset(socket_fd.get(), &tempRead));
            // read length
            int32_t length = unixsocket::readLength(socket_fd.get());
            if (length == -2)
            {
                break;
            }
            else if (length < 0)
            {
                break;
            }
            else if (length == 0)
            {
                continue;
            }

            // read capn proto
            if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
            {
                buffer_size = 1 + length / sizeof(capnp::word);
                proto_buffer = kj::heapArray<capnp::word>(buffer_size);
            }

            auto lengthRead = ::read(socket_fd.get(), proto_buffer.begin(), length);

            if(lengthRead != length)
            {
                break;
            }

            ::send(socket_fd.get(), m_Data->data(), m_Data->size(), 0);

            break;
        }
    }
}
