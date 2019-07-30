/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginResourceManagement.h"

#include "BaseServiceAPI.h"
#include "Logger.h"
#include "RawDataPublisher.h"
#include "Subscriber.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
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
        PluginResourceManagement::PluginResourceManagement() :
            m_contextPtr(Common::ZMQWrapperApi::createContext()),
            m_defaultTimeout(10000),
            m_defaultConnectTimeout(10000)
        {
        }
        PluginResourceManagement::PluginResourceManagement(Common::ZMQWrapperApi::IContextSharedPtr context) :
            m_contextPtr(std::move(context)),
            m_defaultTimeout(10000),
            m_defaultConnectTimeout(10000)
        {
        }

        std::unique_ptr<Common::PluginApi::IBaseServiceApi> PluginResourceManagement::createPluginAPI(
            const std::string& pluginName,
            std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback)
        {
            try
            {
                auto requester = m_contextPtr->getRequester();
                auto replier = m_contextPtr->getReplier();
                setTimeouts(*replier);
                setTimeouts(*requester);

                std::string mng_address =
                    Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
                std::string plugin_address =
                    Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(pluginName);

                requester->connect(mng_address);
                replier->listen(plugin_address);
                // plugin_address starts with ipc:// Remove it.
                std::string plugin_address_file = plugin_address.substr(6);
                // If root owned, we need to ensure the group of the ipc socket is sophos-spl-group
                // so that Management Agent can communicate with the plugin.
                if (::getuid() == 0)
                {
                    Common::FileSystem::filePermissions()->chmod(plugin_address_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); // NOLINT
                    Common::FileSystem::filePermissions()->chown(plugin_address_file, "root", "sophos-spl-group");
                }
                std::unique_ptr<Common::PluginApiImpl::BaseServiceAPI> plugin(
                    new BaseServiceAPI(pluginName, std::move(requester)));

                plugin->setPluginCallback(pluginName, pluginCallback, std::move(replier));

                plugin->registerWithManagementAgent();

                return std::unique_ptr<PluginApi::IBaseServiceApi>(plugin.release());
            }
            catch (std::exception& ex)
            {
                throw PluginApi::ApiException(ex.what());
            }
        }

        std::unique_ptr<Common::PluginApi::IRawDataPublisher> PluginResourceManagement::createRawDataPublisher()
        {
            auto socketPublisher = m_contextPtr->getPublisher();
            setTimeouts(*socketPublisher);
            std::string publishAddressChannel =
                ApplicationConfiguration::applicationPathManager().getPublisherDataChannelAddress();
            socketPublisher->connect(publishAddressChannel);

            // auxiliary subscriber created to prove that pub/sub is setup and in working condition.
            auto socketSubscriber = m_contextPtr->getSubscriber();
            socketSubscriber->setConnectionTimeout(100);
            std::string subscriberAddressChannel =
                Common::ApplicationConfiguration::applicationPathManager().getSubscriberDataChannelAddress();
            socketSubscriber->connect(subscriberAddressChannel);
            std::string checkconnection{ "checkconnection" };
            socketSubscriber->subscribeTo(checkconnection);

            auto mypid = std::to_string(getpid());
            // allow up to 2 seconds for the pub/sub to be setup correctly.
            for (int i = 0; i < 20; i++)
            {
                socketPublisher->write({ checkconnection, mypid });
                try
                {
                    auto arrived_data = socketSubscriber->read();
                    if (arrived_data.at(arrived_data.size() - 1) == mypid)
                    {
                        return std::unique_ptr<PluginApi::IRawDataPublisher>(
                            new RawDataPublisher(std::move(socketPublisher)));
                    }
                }
                catch (std::exception& ex)
                {
                    LOGDEBUG("Setting up publisher intermediate exception: " << ex.what());
                }
            }
            LOGWARN(
                "Failed to setup the publisher as messages are not going to the subscriber. Publisher channel: "
                << publishAddressChannel);
            throw std::runtime_error("Failed to setup publisher");
        }

        std::unique_ptr<Common::PluginApi::ISubscriber> PluginResourceManagement::createSubscriber(
            const std::string& dataCategorySubscription,
            std::shared_ptr<Common::PluginApi::IEventVisitorCallback> eventCallback)
        {
            auto socketSubscriber = m_contextPtr->getSubscriber();
            setTimeouts(*socketSubscriber);
            std::string subscriberAddressChannel =
                Common::ApplicationConfiguration::applicationPathManager().getSubscriberDataChannelAddress();
            socketSubscriber->connect(subscriberAddressChannel);

            return std::unique_ptr<PluginApi::ISubscriber>(new SensorDataSubscriber(
                dataCategorySubscription, std::move(eventCallback), std::move(socketSubscriber)));
        }

        std::unique_ptr<PluginApi::ISubscriber> PluginResourceManagement::createRawSubscriber(
            const std::string& dataCategorySubscription,
            std::shared_ptr<PluginApi::IRawDataCallback> rawDataCallback)
        {
            auto socketSubscriber = m_contextPtr->getSubscriber();
            setTimeouts(*socketSubscriber);
            std::string subscriberAddressChannel =
                Common::ApplicationConfiguration::applicationPathManager().getSubscriberDataChannelAddress();
            socketSubscriber->connect(subscriberAddressChannel);

            return std::unique_ptr<PluginApi::ISubscriber>(new SensorDataSubscriber(
                dataCategorySubscription, std::move(rawDataCallback), std::move(socketSubscriber)));
        }

        void PluginResourceManagement::setDefaultTimeout(int timeoutMs) { m_defaultTimeout = timeoutMs; }

        void PluginResourceManagement::setDefaultConnectTimeout(int timeoutMs) { m_defaultConnectTimeout = timeoutMs; }

        void PluginResourceManagement::setTimeouts(Common::ZeroMQWrapper::ISocketSetup& socket)
        {
            socket.setTimeout(m_defaultTimeout);
            socket.setConnectionTimeout(m_defaultConnectTimeout);
        }

        Common::ZMQWrapperApi::IContextSharedPtr PluginResourceManagement::getSocketContext() { return m_contextPtr; }

    } // namespace PluginApiImpl
} // namespace Common
