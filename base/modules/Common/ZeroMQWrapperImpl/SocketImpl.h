/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SocketHolder.h"

#include <Common/ZeroMQWrapper/IHasFD.h>
#include <Common/ZeroMQWrapper/ISocketSetup.h>

#include <string>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketImpl : public virtual Common::ZeroMQWrapper::ISocketSetup,
                           public virtual Common::ZeroMQWrapper::IHasFD
        {
            struct AppliedSettings
            {
                AppliedSettings() :
                    address(),
                    connectionTimeout(-1),
                    timeout(-1),
                    socketType(0),
                    connectionSetup(ConnectionSetup::NotDefined)
                {
                }
                std::string address;
                int connectionTimeout;
                int timeout;
                int socketType;
                enum class ConnectionSetup
                {
                    Connect,
                    Listen,
                    NotDefined
                };
                ConnectionSetup connectionSetup;
            };

        public:
            int fd() override;

            explicit SocketImpl(ContextHolderSharedPtr context, int type);

            void setTimeout(int timeoutMs) override;

            void setConnectionTimeout(int timeoutMs) override;

            void connect(const std::string& address) override;

            void listen(const std::string& address) override;

            void* skt() { return m_socket.skt(); }

            /** Refresh the socket and reapply the settings.
             * To be used after the socket find to be 'broken'
             *
             */
            void refresh();
            int timeout() const;

        protected:
            ContextHolderSharedPtr m_context;
            AppliedSettings m_appliedSettings;
            SocketHolder m_socket;
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
