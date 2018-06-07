//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_SOCKETHOLDER_H
#define EVEREST_BASE_SOCKETHOLDER_H

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


#endif //EVEREST_BASE_SOCKETHOLDER_H
