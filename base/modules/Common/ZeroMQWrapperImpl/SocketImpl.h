//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_SOCKETIMPL_H
#define EVEREST_BASE_SOCKETIMPL_H

#include "SocketHolder.h"

#include <Common/ZeroMQ_wrapper/ISocketSetup.h>

#include <string>
#include <Common/ZeroMQ_wrapper/IHasFD.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketImpl :
            public virtual Common::ZeroMQ_wrapper::ISocketSetup,
            public virtual Common::ZeroMQ_wrapper::IHasFD
        {
        public:
            int fd() override;

            void setNonBlocking() override;

        public:
            explicit SocketImpl(ContextHolder& context, int type);

            void setTimeout(int timeoutMs) override;

            void connect(const std::string &address) override;

            void listen(const std::string &address) override;
        protected:
            SocketHolder m_socket;
        };
    }
}


#endif //EVEREST_BASE_SOCKETIMPL_H
