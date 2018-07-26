//
// Created by pair on 06/06/18.
//

#pragma once


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


