// Copyright 2023 Sophos Limited. All rights reserved.

#include "PollerFactory.h"

#include "PollerImpl.h"

namespace Common::ZeroMQWrapperImpl
{
    Common::ZeroMQWrapper::IPollerPtr PollerFactory::create() const
    {
        return std::make_unique<PollerImpl>();
    }
} // namespace Common::ZeroMQWrapperImpl

namespace
{
    std::unique_ptr<Common::ZeroMQWrapper::IPollerFactory> globalPollerFactory =
        std::make_unique<Common::ZeroMQWrapperImpl::PollerFactory>();
} // namespace

namespace Common::ZeroMQWrapper
{
    IPollerFactory& pollerFactory()
    {
        return *globalPollerFactory;
    }

    void replacePollerFactory(std::unique_ptr<IPollerFactory> pollerFactory)
    {
        globalPollerFactory = std::move(pollerFactory);
    }

    void restorePollerFactory()
    {
        globalPollerFactory = std::make_unique<Common::ZeroMQWrapperImpl::PollerFactory>();
    }
} // namespace Common::ZeroMQWrapper

Common::ZeroMQWrapper::IPollerPtr Common::ZeroMQWrapper::createPoller()
{
    return Common::ZeroMQWrapper::pollerFactory().create();
}
