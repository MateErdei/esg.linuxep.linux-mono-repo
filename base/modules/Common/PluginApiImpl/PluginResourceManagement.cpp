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
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <sys/stat.h>
#include <unistd.h>

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
        }
        PluginResourceManagement::PluginResourceManagement(Common::ZeroMQWrapper::IContextSharedPtr context)
        : m_contextPtr(std::move(context)), m_defaulTimeout(10000), m_defaultConnectTimeout(10000)
        {

        }

        std::unique_ptr<Common::PluginApi::IBaseServiceApi>
        PluginResourceManagement::createPluginAPI(const std::string &pluginName,
                                                  std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback)
        {
            try
            {
                auto requester = m_contextPtr->getRequester();
                auto replier = m_contextPtr->getReplier();
                setTimeouts(*replier);
                setTimeouts(*requester);

                std::string mng_address = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
                std::string plugin_address = Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(pluginName);

                requester->connect(mng_address);
                replier->listen(plugin_address);

                // If root owned, we need to ensure the group of the ipc socket is sophos-spl-group
                // so that Management Agent can communicate with the plugin.
                if (::getuid() == 0)
                {
                    // plugin_address starts with ipc:// Remove it.
                    std::string plugin_address_file = plugin_address.substr(6);
                    Common::FileSystem::filePermissions()->chmod(plugin_address_file, S_IRWXU | S_IRWXG); //NOLINT
                    Common::FileSystem::filePermissions()->chown(plugin_address_file, "root", "sophos-spl-group");
                }

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
            auto socketPublisher = m_contextPtr->getPublisher();
            setTimeouts(*socketPublisher);
            std::string publishAddressChannel = ApplicationConfiguration::applicationPathManager().getPublisherDataChannelAddress();
            socketPublisher->connect(publishAddressChannel);


            return std::unique_ptr<PluginApi::ISensorDataPublisher>(new SensorDataPublisher(std::move(socketPublisher)));
        }

        std::unique_ptr<Common::PluginApi::ISensorDataSubscriber>
        PluginResourceManagement::createSensorDataSubscriber(const std::string &sensorDataCategorySubscription,
                                                             std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback)
        {
            auto socketSubscriber = m_contextPtr->getSubscriber();
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

        Common::ZeroMQWrapper::IContextSharedPtr PluginResourceManagement::getSocketContext()
        {
            return m_contextPtr;
        }

    }
}
