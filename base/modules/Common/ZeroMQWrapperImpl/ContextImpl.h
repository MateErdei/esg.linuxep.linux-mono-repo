//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H
#define EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H


#include "ContextHolder.h"

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ContextImpl
        {
        public:
            ContextImpl();
            virtual ~ContextImpl();
        private:
            ContextHolder m_context;
        };
    }
}

#endif //EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H
