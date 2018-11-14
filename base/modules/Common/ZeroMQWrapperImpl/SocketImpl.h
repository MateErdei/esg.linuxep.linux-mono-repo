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
            struct AppliedSettings
            {
                AppliedSettings():address(), connectionTimeout(-1), timeout(-1),socketType(0), connectionSetup(ConnectionSetup::NotDefined){}
                std::string address;
                int connectionTimeout;
                int timeout;
                int socketType;
                enum class ConnectionSetup {Connect, Listen,NotDefined} ;
                ConnectionSetup connectionSetup;
            };

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
            void refresh() override;
            int timeout() const;
        protected:
            ContextHolder & m_referenceContext;
            AppliedSettings m_appliedSettings;
            SocketHolder m_socket;
        };
    }
}



