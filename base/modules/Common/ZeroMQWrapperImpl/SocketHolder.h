//
// Created by pair on 06/06/18.
//

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
            explicit SocketHolder(ContextHolder& context, int type);
            ~SocketHolder();
            void* skt();
            void reset(void* zmq_socket = nullptr);
        private:
            void* m_socket;
        };
    }
}



