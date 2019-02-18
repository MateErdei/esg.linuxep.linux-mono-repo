/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ContextHolder.h"

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketHolder final
        {
        public:
            explicit SocketHolder(void* zmq_socket = nullptr);
            explicit SocketHolder(ContextHolderSharedPtr& context, int type);
            ~SocketHolder();
            void* skt();
            void reset(void* zmq_socket = nullptr);
            void reset(ContextHolderSharedPtr& context, int type);

        private:
            void* m_socket;
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
