/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServerConnectionThread.h"

#include "ScanRequest.capnp.h"

#include "unixsocket/SocketUtils.h"

#include <common/FDUtils.h>

#include <cassert>
#include <utility>

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

FakeDetectionServer::FakeServerConnectionThread::FakeServerConnectionThread(
    datatypes::AutoFd socketFd,
    std::shared_ptr<std::vector<uint8_t>> Data)
    :  m_socketFd(std::move(socketFd))
    , m_data(std::move(Data))
{
}

void FakeDetectionServer::FakeServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_socketFd));
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    struct pollfd fds[] {
        { .fd = socket_fd.get(), .events = POLLIN, .revents = 0 }, // socket FD
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
    };

    while (true)
    {
        auto ret = ::ppoll(fds, std::size(fds), nullptr, nullptr);
        if (ret < 0)
        {
            break;
        }
        assert(ret > 0);

        if ((fds[1].revents & POLLIN) != 0)
        {
            break;
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
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

            ::send(socket_fd.get(), m_data->data(), m_data->size(), 0);

            break;
        }
    }
}
