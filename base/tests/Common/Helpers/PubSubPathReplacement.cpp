/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PubSubPathReplacement.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/ZeroMQWrapperImpl/ProxyImpl.h>
#include <Common/ZeroMQWrapperImpl/ContextImpl.h>


namespace
{
    //this is used to override the normal ApplicationPathManager methods for testing purposes
    class ForTestPathManager: public Common::ApplicationConfigurationImpl::ApplicationPathManager
    {
        std::string getPublisherDataChannelAddress() const override
        {
            return "inproc://datachannelpub.ipc";
        }
        std::string getSubscriberDataChannelAddress() const override
        {
            return "inproc://datachannelsub.ipc";
        }

    };
}

namespace Tests
{


    PubSubPathReplacement::PubSubPathReplacement()
    {
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>( new ForTestPathManager() ));

        m_context = std::make_shared<Common::ZeroMQWrapperImpl::ContextImpl>();
        m_proxy = m_context->getProxy(
                Common::ApplicationConfiguration::applicationPathManager().getPublisherDataChannelAddress(),
                Common::ApplicationConfiguration::applicationPathManager().getSubscriberDataChannelAddress()
                );
        m_proxy->start();
    }

    PubSubPathReplacement::~PubSubPathReplacement()
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
    }


    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> PubSubPathReplacement::createPluginResourceManagement()
    {
        return std::unique_ptr<Common::PluginApi::IPluginResourceManagement>(new Common::PluginApiImpl::PluginResourceManagement(m_context));
    }

}
