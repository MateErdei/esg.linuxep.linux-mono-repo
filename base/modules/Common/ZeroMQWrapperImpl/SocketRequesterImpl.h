//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_SOCKETREQUESTER_H
#define EVEREST_BASE_SOCKETREQUESTER_H

#include "SocketImpl.h"

#include <Common/ZeroMQ_wrapper/ISocketRequester.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketRequesterImpl : public SocketImpl, public virtual Common::ZeroMQ_wrapper::ISocketRequester
        {
        public:
            explicit SocketRequesterImpl(ContextHolder& context);

            std::vector<std::string> read() override;

            void write(const std::vector<std::string> &data) override;

        public:

        };
    }
}

#endif //EVEREST_BASE_SOCKETREQUESTER_H
