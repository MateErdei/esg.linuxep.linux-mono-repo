/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/IProxy.h>

namespace Tests
{
    class PubSubPathReplacement
    {
    public:
        PubSubPathReplacement();
        ~PubSubPathReplacement();

        std::unique_ptr<Common::PluginApi::IPluginResourceManagement> createPluginResourceManagement();

    private:
        Common::ZeroMQWrapper::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::IProxyPtr m_proxy;
    };

} // namespace Tests
