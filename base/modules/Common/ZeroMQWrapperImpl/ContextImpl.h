//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H
#define EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H

#include "ContextHolder.h"

#include <Common/ZeroMQ_wrapper/IContext.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ContextImpl : public virtual Common::ZeroMQ_wrapper::IContext
        {
        public:
            ContextImpl() = default;
            ~ContextImpl() override = default;

            std::unique_ptr<ZeroMQ_wrapper::ISocketSubscriber> getSubscriber() override;

            std::unique_ptr<ZeroMQ_wrapper::ISocketPublisher> getPublisher() override;

            Common::ZeroMQ_wrapper::ISocketRequesterPtr getRequester() override;

            std::unique_ptr<ZeroMQ_wrapper::ISocketReplier> getReplier() override;

        private:
            ContextHolder m_context;
        };
    }
}

#endif //EVEREST_BASE_COMMON_ZEROMQWRAPPERIMPL_CONTEXT_H
