//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_SOCKETREPLIERIMPL_H
#define EVEREST_BASE_SOCKETREPLIERIMPL_H

#include "SocketImpl.h"

#include <Common/ZeroMQ_wrapper/ISocketReplier.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketReplierImpl :
                public SocketImpl,
                virtual public Common::ZeroMQ_wrapper::ISocketReplier
        {
        public:
            explicit SocketReplierImpl(ContextHolder& context);

            std::vector<std::string> read() override;

            void write(const std::vector<std::string> &data) override;
        };
    }
}

#endif //EVEREST_BASE_SOCKETREPLIERIMPL_H
