/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginResourceManagement.h"
#include "BaseServiceAPI.h"
#include "SensorDataPublisher.h"
#include "SensorDataSubscriber.h"

#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <sys/stat.h>

namespace Common
{
    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> Common::PluginApi::createPluginResourceManagement()
    {
        return std::unique_ptr<IPluginResourceManagement>(new Common::PluginApiImpl::PluginResourceManagement());
    }

    namespace PluginApiImpl
    {

        PluginResourceManagement::PluginResourceManagement()
        : m_contextPtr( Common::ZeroMQWrapper::createContext()), m_defaulTimeout(10000), m_defaultConnectTimeout(10000)
        {
            m_context = m_contextPtr.get();
        }
        PluginResourceManagement::PluginResourceManagement(Common::ZeroMQWrapper::IContext * context)
        : m_contextPtr(), m_context(context), m_defaulTimeout(10000), m_defaultConnectTimeout(10000)
        {

        }

        std::unique_ptr<Common::PluginApi::IBaseServiceApi>
        PluginResourceManagement::createPluginAPI(const std::string &pluginName,
                                                  std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback)
        {
            try
            {
                auto requester = m_context->getRequester();
                auto replier = m_context->getReplier();
                setTimeouts(*replier);
                setTimeouts(*requester);

                std::string mng_address = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
                std::string plugin_address = Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(pluginName);

                requester->connect(mng_address);
                replier->listen(plugin_address);

                std::string plugin_address_file = plugin_address.substr(6);
                Common::FileSystem::fileSystem()->chownChmod(plugin_address_file, "root", "sophos-spl-group", S_IRWXU | S_IRGRP | S_IXGRP);

                std::unique_ptr<Common::PluginApiImpl::BaseServiceAPI> plugin( new BaseServiceAPI(pluginName, std::move(requester)));


                plugin->setPluginCallback(pluginName, pluginCallback, std::move(replier));

                plugin->registerWithManagementAgent();

                return std::unique_ptr<PluginApi::IBaseServiceApi>( plugin.release());

            }
            catch(std::exception & ex)
            {
                throw PluginApi::ApiException(ex.what());
            }
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

        Common::ZeroMQWrapper::IContext &PluginResourceManagement::getSocketContext()
        {
            return *m_context;
        }

    }
}
