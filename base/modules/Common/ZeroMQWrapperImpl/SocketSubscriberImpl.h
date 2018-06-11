//
// Created by pair on 08/06/18.
//

#ifndef EVEREST_BASE_SOCKETSUBSCRIBERIMPL_H
#define EVEREST_BASE_SOCKETSUBSCRIBERIMPL_H


#include "SocketImpl.h"

#include <Common/ZeroMQWrapper/ISocketSubscriber.h>

#include <zmq.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketSubscriberImpl : public SocketImpl,
            virtual public Common::ZeroMQWrapper::ISocketSubscriber
        {
        public:
            explicit SocketSubscriberImpl(ContextHolder& context)
                : SocketImpl(context, ZMQ_SUB)
            {}

            /**
             * Read a subscribed event from the socket
             * @return
             */
            std::vector<std::string> read() override;

            /**
             * Set the subscription for this socket.
             * @param subject
             */
            void subscribeTo(const std::string &subject) override;
        };

    }
}


#endif //EVEREST_BASE_SOCKETSUBSCRIBERIMPL_H
