// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapperImpl/ContextHolder.h"

namespace Common::ZMQWrapperApiImpl
{
    class ContextImpl : public virtual Common::ZMQWrapperApi::IContext
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
        Common::ZeroMQWrapperImpl::ContextHolderSharedPtr m_context;
    };
} // namespace Common::ZMQWrapperApiImpl
