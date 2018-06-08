//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H
#define EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H

#include "ContextHolder.h"

#include <Common/ZeroMQWrapper/IContext.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ContextImpl : public virtual Common::ZeroMQWrapper::IContext
        {
        public:
            ContextImpl() = default;
            ~ContextImpl() override = default;

            Common::ZeroMQWrapper::ISocketSubscriberPtr getSubscriber() override;

            Common::ZeroMQWrapper::ISocketPublisherPtr getPublisher() override;

            Common::ZeroMQWrapper::ISocketRequesterPtr getRequester() override;

            Common::ZeroMQWrapper::ISocketReplierPtr getReplier() override;

        private:
            ContextHolder m_context;
        };
    }
}

#endif //EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H
