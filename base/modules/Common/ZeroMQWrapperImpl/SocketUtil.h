/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SocketHolder.h"

#include <string>
#include <vector>

namespace Common::ZeroMQWrapperImpl
{
    class SocketUtil
    {
    public:
        /**
         * Delete the default constructor to force this to be a static class
         */
        SocketUtil() = delete;

        using data_t = std::vector<std::string>;

        /**
         * Read a multi-part message from a socket.
         * @return A vector with the message
         */
        static data_t read(SocketHolder&);

        /**
         * Send a multi-part message to a socket
         * @param data The multi-part message to send.
         */
        static void write(SocketHolder&, const data_t& data);

        /**
         * Do a bind from a socket to an address, to listen for connections.
         * @param address
         */
        static void listen(SocketHolder& socket, const std::string& address);

        /**
         * Connect a socket to an address, to publish information to.
         * @param address
         */
        static void connect(SocketHolder& socket, const std::string& address);

        /**
         * Set the timeouts (both send and receive for a 0MQ socket.
         * @param socket
         * @param timeoutMs
         */
        static void setTimeout(SocketHolder& socket, int timeoutMs);

        /**
         * Set the timeout for 0MQ connections
         * @param socket
         * @param timeoutMs
         */
        static void setConnectionTimeout(Common::ZeroMQWrapperImpl::SocketHolder& socket, int timeoutMs);

        /**
         * Monitor the socket for incoming data and return as soon as there is data available or throw
         * IIPCTimeoutException
         * @param socket
         * @param timeoutMs
         */
        static void checkIncomingData(Common::ZeroMQWrapperImpl::SocketHolder& socket, int timeoutMs);
    };
} // namespace Common::ZeroMQWrapperImpl
