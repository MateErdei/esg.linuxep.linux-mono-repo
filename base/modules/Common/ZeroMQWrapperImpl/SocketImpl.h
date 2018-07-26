/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "SocketHolder.h"

#include <Common/ZeroMQWrapper/ISocketSetup.h>

#include <string>
#include <Common/ZeroMQWrapper/IHasFD.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketImpl :
            public virtual Common::ZeroMQWrapper::ISocketSetup,
            public virtual Common::ZeroMQWrapper::IHasFD
        {
        public:
            int fd() override;

            explicit SocketImpl(ContextHolder& context, int type);

            void setTimeout(int timeoutMs) override;

            void setConnectionTimeout(int timeoutMs) override;

            void connect(const std::string &address) override;

            void listen(const std::string &address) override;

            void* skt()
            {
                return m_socket.skt();
            }
        protected:
            SocketHolder m_socket;
        };
    }
}



