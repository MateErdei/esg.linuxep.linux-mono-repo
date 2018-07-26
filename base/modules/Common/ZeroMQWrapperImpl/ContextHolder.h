/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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



