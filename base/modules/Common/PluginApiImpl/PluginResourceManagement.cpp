// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "PluginResourceManagement.h"

#include "BaseServiceAPI.h"
#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketSubscriber.h"

#include <sys/stat.h>

#include <unistd.h>

namespace Common
{
    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> Common::PluginApi::createPluginResourceManagement()
    {
        return std::unique_ptr<IPluginResourceManagement>(new Common::PluginApiImpl::PluginResourceManagement());
    }
} // namespace Common

namespace Common::PluginApiImpl
{
    void PluginResourceManagement::setupReplier(
        Common::ZeroMQWrapper::ISocketReplier& replier,
        const std::string& pluginName,
        int defaultTimeout,
        int connectTimeout)
    {
        setTimeouts(replier, defaultTimeout, connectTimeout);
        std::string pluginAddress =
            Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(pluginName);
        LOGSUPPORT("Setup ipc replier to connect to " << pluginAddress);
        replier.listen(pluginAddress);

        // If root owned, we need to ensure the group of the ipc socket is sophos-spl-group
        // so that Management Agent can communicate with the plugin.
        std::string pluginAddressFile = pluginAddress.substr(6);
        if (::getuid() == 0)
        {
            LOGSUPPORT("Setup ipc replier permissions: " << pluginAddressFile);
            // plugin_address starts with ipc:// Remove it.
            Common::FileSystem::filePermissions()->chown(pluginAddressFile, "root", sophos::group());
        }

        if (::getuid() != Common::FileSystem::filePermissions()->getUserId(sophos::user()))
        {
            Common::FileSystem::filePermissions()->chmod(
                pluginAddressFile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        }
    }
    void PluginResourceManagement::setupRequester(
        Common::ZeroMQWrapper::ISocketRequester& requester,
        const std::string& pluginName,
        int defaultTimeout,
        int connectTimeout)
    {
        std::string pluginSocketAdd =
            Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(pluginName);
        LOGSUPPORT("Setup ipc requester to connect to " << pluginSocketAdd);
        setTimeouts(requester, defaultTimeout, connectTimeout);
        requester.connect(pluginSocketAdd);
    }

    PluginResourceManagement::PluginResourceManagement() :
        m_contextPtr(Common::ZMQWrapperApi::createContext()), m_defaultTimeout(6000), m_defaultConnectTimeout(10000)
    {
    }
    
    PluginResourceManagement::PluginResourceManagement(Common::ZMQWrapperApi::IContextSharedPtr context) :
        m_contextPtr(std::move(context)), m_defaultTimeout(6000), m_defaultConnectTimeout(10000)
    {
    }

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> PluginResourceManagement::createPluginAPI(
        const std::string& pluginName,
        std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback)
    {
        try
        {
            auto requester = m_contextPtr->getRequester();
            setTimeouts(*requester);

            std::string mng_address =
                Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();

            requester->connect(mng_address);

            std::unique_ptr<Common::PluginApiImpl::BaseServiceAPI> plugin(
                new BaseServiceAPI(pluginName, std::move(requester)));

            auto replier = m_contextPtr->getReplier();
            setupReplier(*replier, pluginName, m_defaultTimeout, m_defaultConnectTimeout);

            plugin->setPluginCallback(pluginName, pluginCallback, std::move(replier));

            plugin->registerWithManagementAgent();

            LOGSUPPORT("Restoring telemetry from disk for plugin: " << pluginName);
            Common::Telemetry::TelemetryHelper::getInstance().restore(pluginName);

            return std::unique_ptr<PluginApi::IBaseServiceApi>(plugin.release());
        }
        catch (std::exception& ex)
        {
            throw PluginApi::ApiException(ex.what());
        }
    }

    void PluginResourceManagement::setDefaultTimeout(int timeoutMs)
    {
        m_defaultTimeout = timeoutMs;
    }

    void PluginResourceManagement::setDefaultConnectTimeout(int timeoutMs)
    {
        m_defaultConnectTimeout = timeoutMs;
    }

    void PluginResourceManagement::setTimeouts(
        Common::ZeroMQWrapper::ISocketSetup& socket,
        int defaultTimeout,
        int connectTimeout)
    {
        socket.setTimeout(defaultTimeout);
        socket.setConnectionTimeout(connectTimeout);
    }
    void PluginResourceManagement::setTimeouts(Common::ZeroMQWrapper::ISocketSetup& socket)
    {
        setTimeouts(socket, m_defaultTimeout, m_defaultConnectTimeout);
    }

    Common::ZMQWrapperApi::IContextSharedPtr PluginResourceManagement::getSocketContext()
    {
        return m_contextPtr;
    }
} // namespace Common::PluginApiImpl
