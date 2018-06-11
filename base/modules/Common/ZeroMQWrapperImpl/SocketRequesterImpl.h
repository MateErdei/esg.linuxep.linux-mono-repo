//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_SOCKETREQUESTER_H
#define EVEREST_BASE_SOCKETREQUESTER_H

#include "SocketImpl.h"

#include <Common/ZeroMQWrapper/ISocketRequester.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketRequesterImpl : public SocketImpl, public virtual Common::ZeroMQWrapper::ISocketRequester
        {
        public:
            explicit SocketRequesterImpl(ContextHolder& context);

            std::vector<std::string> read() override;

            void write(const std::vector<std::string> &data) override;

        };
    }
}

#endif //EVEREST_BASE_SOCKETREQUESTER_H
