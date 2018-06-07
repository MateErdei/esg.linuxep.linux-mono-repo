//
// Created by pair on 07/06/18.
//

#include "SocketRequesterImpl.h"

#include <zmq.h>
#include <cassert>

using namespace Common::ZeroMQWrapperImpl;

SocketRequesterImpl::SocketRequesterImpl(Common::ZeroMQWrapperImpl::ContextHolder &context)
    : SocketImpl(context, ZMQ_REQ)
{
}

std::vector<std::string> Common::ZeroMQWrapperImpl::SocketRequesterImpl::read()
{
    void* socket = m_socket.skt();
    std::vector<std::string> res;
    int64_t more;
    size_t more_size = sizeof(more);
    do {
        /* Create an empty ØMQ message to hold the message part */
        zmq_msg_t part;
        int rc = zmq_msg_init(&part);
        assert (rc == 0);
        /* Block until a message is available to be received from socket */
        rc = zmq_msg_recv(&part, socket, 0);
        assert (rc >= 0);

        void* contents = zmq_msg_data(&part);
        if (contents != nullptr)
        {
            res.emplace_back(reinterpret_cast<char*>(contents),static_cast<std::string::size_type>(rc));
        }

        /* Determine if more message parts are to follow */
        rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
        assert (rc == 0);
        zmq_msg_close (&part);
    } while (more);
    return res;
}

void Common::ZeroMQWrapperImpl::SocketRequesterImpl::write(const std::vector<std::string> &data)
{
    void* socket = m_socket.skt();
    // Need to iterate through everything other than the last element
    for (int i=0;i<data.size()-1;++i)
    {
        const std::string & ref = data[i];
        zmq_send(socket, ref.data(), ref.size(), ZMQ_SNDMORE);
    }
    const std::string& ref = data[data.size()-1];
    zmq_send(socket, ref.data(), ref.size(), 0);
}
