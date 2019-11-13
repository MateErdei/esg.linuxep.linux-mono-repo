/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketUtil.h"

#include "ZeroMQTimeoutException.h"
#include "ZeroMQWrapperException.h"

#include <Common/Exceptions/Print.h>

#include <sstream>
#include <zmq.h>

std::vector<std::string> Common::ZeroMQWrapperImpl::SocketUtil::read(
    Common::ZeroMQWrapperImpl::SocketHolder& socketHolder)
{
    void* socket = socketHolder.skt();
    std::vector<std::string> res;
    int64_t more;
    size_t more_size = sizeof(more);
    do
    {
        // Create an empty ØMQ message to hold the message part
        zmq_msg_t part;
        int rc = zmq_msg_init(&part);
        if (rc != 0)
        {
            throw ZeroMQWrapperException("Failed to init msg while reading");
        }
        /* Block until a message is available to be received from socket */
        rc = zmq_msg_recv(&part, socket, 0);
        if (rc < 0)
        {
            throw ZeroMQWrapperException("Failed to receive message component");
        }

        void* contents = zmq_msg_data(&part);
        if (contents != nullptr)
        {
            res.emplace_back(reinterpret_cast<char*>(contents), static_cast<std::string::size_type>(rc));
        }

        // Determine if more message parts are to follow
        rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
        if (rc != 0)
        {
            throw ZeroMQWrapperException("Failed to get ZMQ_RCVMORE setting from socket");
        }
        zmq_msg_close(&part);
    } while (more);
    return res;
}

void Common::ZeroMQWrapperImpl::SocketUtil::write(
    Common::ZeroMQWrapperImpl::SocketHolder& socketHolder,
    const std::vector<std::string>& data)
{
    if (data.empty())
    {
        return;
    }
    void* socket = socketHolder.skt();
    int rc;
    // Need to iterate through everything other than the last element
    for (size_t i = 0; i < data.size() - 1; ++i)
    {
        const std::string& ref = data[i];
        errno = 0;
        rc = zmq_send(socket, ref.data(), ref.size(), ZMQ_SNDMORE);
        if (rc < 0)
        {
            std::ostringstream ost;
            ost << "Failed to send data block: errno=" << errno << ": " << zmq_strerror(errno);
            if (errno == EAGAIN)
            {
                throw ZeroMQTimeoutException(ost.str());
            }
            throw ZeroMQWrapperException(ost.str());
        }
    }
    const std::string& ref = data[data.size() - 1];
    errno = 0;
    rc = zmq_send(socket, ref.data(), ref.size(), 0);
    if (rc < 0)
    {
        std::ostringstream ost;
        ost << "Failed to send final data block: errno=" << errno << ": " << zmq_strerror(errno);
        if (errno == EAGAIN)
        {
            throw ZeroMQTimeoutException(ost.str());
        }
        throw ZeroMQWrapperException(ost.str());
    }
}

void Common::ZeroMQWrapperImpl::SocketUtil::listen(
    Common::ZeroMQWrapperImpl::SocketHolder& socket,
    const std::string& address)
{
    int rc = zmq_bind(socket.skt(), address.c_str());
    if (rc != 0)
    {
        throw ZeroMQWrapperException(std::string("Failed to bind to ") + address);
    }
}

void Common::ZeroMQWrapperImpl::SocketUtil::connect(SocketHolder& socket, const std::string& address)
{
    int rc = zmq_connect(socket.skt(), address.c_str());
    if (rc != 0)
    {
        throw ZeroMQWrapperException(std::string("Failed to connect to ") + address);
    }
}

void Common::ZeroMQWrapperImpl::SocketUtil::setTimeout(Common::ZeroMQWrapperImpl::SocketHolder& socket, int timeoutMs)
{
    int rc = zmq_setsockopt(socket.skt(), ZMQ_RCVTIMEO, &timeoutMs, sizeof(timeoutMs));
    if (rc != 0)
    {
        throw ZeroMQWrapperException("Failed to set timeout for receiving");
    }
    rc = zmq_setsockopt(socket.skt(), ZMQ_SNDTIMEO, &timeoutMs, sizeof(timeoutMs));
    if (rc != 0)
    {
        throw ZeroMQWrapperException("Failed to set timeout for sending");
    }
}

void Common::ZeroMQWrapperImpl::SocketUtil::setConnectionTimeout(
    Common::ZeroMQWrapperImpl::SocketHolder& socket,
    int timeoutMs)
{
    int rc = zmq_setsockopt(socket.skt(), ZMQ_LINGER, &timeoutMs, sizeof(timeoutMs));
    if (rc != 0)
    {
        throw ZeroMQWrapperException("Failed to set connection timeout");
    }
    int currTimeout;
    size_t timeoutExpectedSize = sizeof(currTimeout);
    if (zmq_getsockopt(socket.skt(), ZMQ_RCVTIMEO, &currTimeout, &timeoutExpectedSize) != 0)
    {
        throw ZeroMQWrapperException("Failed to retrieve current timeout");
    };
    if (currTimeout == -1)
    {
        setTimeout(socket, timeoutMs);
    }
}

void Common::ZeroMQWrapperImpl::SocketUtil::checkIncomingData(
    Common::ZeroMQWrapperImpl::SocketHolder& socket,
    int timeoutMs)
{
    zmq_pollitem_t items[1] = { { socket.skt(), 0, ZMQ_POLLIN, 0 } };

    int attempts = 3;
    while (attempts-- >= 0)
    {
        int rc = zmq_poll(items, 1, timeoutMs);

        if (rc > 0)
            return;
        if (rc == 0)
        {
            throw Common::ZeroMQWrapper::IIPCTimeoutException("No incoming data");
        }

        if (rc < 0)
        {
            if (zmq_errno() == EINTR)
            {
                continue;
            }
            std::string info = "Failed to poll. ";
            int z_err = zmq_errno();
            if (z_err != 0)
            {
                info += zmq_strerror(z_err);
            }

            throw ZeroMQPollerException(info);
        }
    }
}
