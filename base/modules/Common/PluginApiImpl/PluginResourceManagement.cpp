/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginResourceManagement.h"
#include "PluginApiImpl.h"
#include "SensorDataPublisher.h"
#include "SensorDataSubscriber.h"

#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/ZeroMQWrapper/ISocketSubscriber.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"


namespace Common
{
    namespace PluginApiImpl
    {

        PluginResourceManagement::PluginResourceManagement()
        : m_context( Common::ZeroMQWrapper::createContext()), m_defaulTimeout(-1), m_defaultConnectTimeout(-1)
        {
        }

        std::unique_ptr<Common::PluginApi::IPluginApi>
        PluginResourceManagement::createPluginAPI(const std::string &pluginName,
                                                  std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback)
        {
            auto requester = m_context->getRequester();
            auto replier = m_context->getReplier();
            setTimeouts(*replier);
            setTimeouts(*requester);

            std::string mng_address = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
            std::string plugin_address = Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(pluginName);

            requester->connect(mng_address);
            replier->listen(plugin_address);

            std::unique_ptr<Common::PluginApiImpl::PluginApiImpl> plugin( new PluginApiImpl(pluginName, std::move(requester)));


            plugin->setPluginCallback(pluginCallback, std::move(replier));

            return std::unique_ptr<PluginApi::IPluginApi>( plugin.release());
        }

        std::unique_ptr<Common::PluginApi::ISensorDataPublisher>
        PluginResourceManagement::createSensorDataPublisher(const std::string &pluginName)
        {
            auto socketPublisher = m_context->getPublisher();
            setTimeouts(*socketPublisher);
            std::string publishAddressChannel = ApplicationConfiguration::applicationPathManager().getPublisherDataChannelAddress();
            socketPublisher->connect(publishAddressChannel);

            return std::unique_ptr<PluginApi::ISensorDataPublisher>(new SensorDataPublisher(std::move(socketPublisher)));
        }

        std::unique_ptr<Common::PluginApi::ISensorDataSubscriber>
        PluginResourceManagement::createSensorDataSubscriber(const std::string &sensorDataCategorySubscription,
                                                             std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback)
        {
            auto socketSubscriber = m_context->getSubscriber();
            setTimeouts(*socketSubscriber);
            std::string subscriberAddressChannel = Common::ApplicationConfiguration::applicationPathManager().getSubscriberDataChannelAddress();
            socketSubscriber->listen(subscriberAddressChannel);

            return std::unique_ptr<PluginApi::ISensorDataSubscriber>( new SensorDataSubscriber( sensorDataCategorySubscription, sensorDataCallback, std::move(socketSubscriber) ));
        }

        void PluginResourceManagement::setDefaultTimeout(int timeoutMs)
        {
            m_defaulTimeout = timeoutMs;

        }

        void PluginResourceManagement::setDefaultConnectTimeout(int timeoutMs)
        {
            m_defaultConnectTimeout = timeoutMs;
        }

        void PluginResourceManagement::setTimeouts(Common::ZeroMQWrapper::ISocketSetup &socket)
        {
            socket.setTimeout(m_defaulTimeout);
            socket.setConnectionTimeout(m_defaultConnectTimeout);
        }

        Common::ZeroMQWrapper::IContext &PluginResourceManagement::socketContext()
        {
            return *m_context;
        }
    }
}
