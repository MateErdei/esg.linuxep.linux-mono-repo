//
// Created by pair on 07/06/18.
//

#pragma once



#include "SocketImpl.h"

#include <Common/ZeroMQWrapper/ISocketPublisher.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketPublisherImpl :
                public SocketImpl,
                public virtual Common::ZeroMQWrapper::ISocketPublisher
        {
        public:
            explicit SocketPublisherImpl(ContextHolder& context);

            void write(const std::vector<std::string> &data) override;
        };
    }
}



