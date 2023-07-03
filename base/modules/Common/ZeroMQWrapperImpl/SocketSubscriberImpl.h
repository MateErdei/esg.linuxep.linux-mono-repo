/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SocketImpl.h"

#include "modules/Common/ZeroMQWrapper/ISocketSubscriber.h"

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketSubscriberImpl : public SocketImpl, virtual public Common::ZeroMQWrapper::ISocketSubscriber
        {
        public:
            explicit SocketSubscriberImpl(ContextHolderSharedPtr context);

            /**
             * Read a subscribed event from the socket
             * @return
             */
            std::vector<std::string> read() override;

            /**
             * Set the subscription for this socket.
             * @param subject
             */
            void subscribeTo(const std::string& subject) override;
        };

    } // namespace ZeroMQWrapperImpl
} // namespace Common
