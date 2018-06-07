//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_ISOCKETSETUP_H
#define EVEREST_BASE_ISOCKETSETUP_H

namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class ISocketSetup
        {
        public:
            virtual ~ISocketSetup() = default;
            /**
             * Set the socket receive and send timeout
             * @param timeoutMs
             */
            virtual void setTimeout(int timeoutMs) = 0;

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

#endif //EVEREST_BASE_ISOCKETSETUP_H
