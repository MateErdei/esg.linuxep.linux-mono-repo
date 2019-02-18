/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
            ContextImpl();
            ~ContextImpl() override;

            Common::ZeroMQWrapper::ISocketSubscriberPtr getSubscriber() override;

            Common::ZeroMQWrapper::ISocketPublisherPtr getPublisher() override;

            Common::ZeroMQWrapper::ISocketRequesterPtr getRequester() override;

            Common::ZeroMQWrapper::ISocketReplierPtr getReplier() override;

            Common::ZeroMQWrapper::IProxyPtr getProxy(const std::string& frontend, const std::string& backend) override;

        private:
            ContextHolderSharedPtr m_context;
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
