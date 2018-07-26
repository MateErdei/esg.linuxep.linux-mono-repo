//
// Created by pair on 06/06/18.
//

#pragma once



namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ContextHolder final
        {
        public:
            ContextHolder();
            ~ContextHolder();
            void* ctx();
            void reset();
        private:
            void* m_context;
        };
    }
}



