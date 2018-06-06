//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_SOCKETHOLDER_H
#define EVEREST_BASE_SOCKETHOLDER_H


namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketHolder final
        {
        public:
            SocketHolder(void* zmq_socket = nullptr);
            ~SocketHolder();
            void* skt();
            void reset(void* zmq_socket = nullptr);
        private:
            void* m_socket;
        };
    }
}


#endif //EVEREST_BASE_SOCKETHOLDER_H
