//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_SOCKETREPLIERIMPL_H
#define EVEREST_BASE_SOCKETREPLIERIMPL_H

#include "SocketImpl.h"

#include <Common/ZeroMQWrapper/ISocketReplier.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketReplierImpl :
                public SocketImpl,
                virtual public Common::ZeroMQWrapper::ISocketReplier
        {
        public:
            explicit SocketReplierImpl(ContextHolder& context);

            std::vector<std::string> read() override;

            void write(const std::vector<std::string> &data) override;
        };
    }
}

#endif //EVEREST_BASE_SOCKETREPLIERIMPL_H
