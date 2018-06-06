//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_CONTEXTHOLDER_H
#define EVEREST_BASE_CONTEXTHOLDER_H


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
        private:
            void* m_context;
        };
    }
}


#endif //EVEREST_BASE_CONTEXTHOLDER_H
