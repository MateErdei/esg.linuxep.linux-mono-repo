/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketSetup
        {
        public:
            virtual ~ISocketSetup() = default;
            /**
             * Set the socket receive and send timeout
             *
             * For ZeroMQ implementation see See http://api.zeromq.org/3-0:zmq-setsockopt ZMQ_RCVTIMEO and ZMQ_SNDTIMEO
             *
             * timeoutMs can be -1 to disable timeouts.
             *
             * Default: -1
             *
             * @param timeoutMs Milliseconds to wait to receive or send messages on the socket, or -1 to wait forever.
             */
            virtual void setTimeout(int timeoutMs) = 0;

            /**
             * Set the socket connection timeout
             *
             * For ZeroMQ implementation see See http://api.zeromq.org/3-0:zmq-setsockopt ZMQ_LINGER
             *
             * timeoutMs can be -1 to disable timeouts.
             *
             * It is not possible to have an infinite (-1) timeout (::setTimeout) with a finite connection timeout
             * (::setConnectionTimeout). For this reason, the setConnectionTimeout will check the current Timeout
             * and if set to (-1) it will change it to match the ConnectionTimeout given.
             *
             * Default: -1
             *
             * @param timeoutMs Milliseconds to wait to receive or send messages on the socket, or -1 to wait forever.
             */
            virtual void setConnectionTimeout(int timeoutMs) = 0;

            /**
             * Connect as a client socket
             * @param address
             */

            virtual void connect(const std::string& address) = 0;

            /**
             * Listen for connections
             * @param address
             */
            virtual void listen(const std::string& address) = 0;
        };
    }
}


