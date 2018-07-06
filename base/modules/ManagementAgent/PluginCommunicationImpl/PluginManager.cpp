/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "ApplicationConfiguration/IApplicationPathManager.h"
#include "IPluginCommunicationException.h"
#include "PluginManager.h"
#include "PluginProxy.h"

namespace ManagementAgent
{
namespace PluginCommunicationImpl
{
    PluginManager::PluginManager() :
    m_serverCallbackHandler(), m_context(Common::ZeroMQWrapper::createContext()), m_defaultTimeout(-1), m_defaultConnectTimeout(-1)
    {}

    PluginManager::~PluginManager()
    {
        if( m_serverCallbackHandler)
        {
            m_serverCallbackHandler->stop();
        }

    }

    void PluginManager::setServerCallback(
            std::shared_ptr<PluginCommunication::IPluginServerCallback> serverCallback, Common::ZeroMQWrapper::ISocketReplierPtr replier)
    {
        m_serverCallbackHandler.reset( new PluginServerCallbackHandler( std::move(replier), serverCallback) );
        m_serverCallbackHandler->start();
    }


    void PluginManager::setDefaultTimeout(int timeoutMs)
    {
        m_defaultTimeout = timeoutMs;

    }

    void PluginManager::setDefaultConnectTimeout(int timeoutMs)
    {
        m_defaultConnectTimeout = timeoutMs;
    }


    void PluginManager::applyNewPolicy(const std::string &appId, const std::string &policyXml)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        for (auto &proxy : m_RegisteredPlugins)
        {
            if (proxy.second->hasAppId(appId))
            {
                proxy.second->applyNewPolicy(appId, policyXml);
            }
        }
    }

    void PluginManager::doAction(const std::string &appId, const std::string &actionXml)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        for (auto &proxy : m_RegisteredPlugins)
        {
            if (proxy.second->hasAppId(appId))
            {
                proxy.second->doAction(appId, actionXml);
            }
        }
    }

    Common::PluginProtocol::StatusInfo PluginManager::getStatus(const std::string & pluginName)
    {
        return getPlugin(pluginName)->getStatus();
    }

    std::string PluginManager::getTelemetry(const std::string & pluginName)
    {
        return getPlugin(pluginName)->getTelemetry();
    }

    void PluginManager::setAppIds(const std::string &pluginName, const std::vector<std::string> &appIds)
    {
        getPlugin(pluginName)->setAppIds(appIds);
    }

    void PluginManager::registerPlugin(const std::string &pluginName)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        std::string pluginSocketAdd = Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(pluginName);
        auto requester = m_context->getRequester();
        setTimeouts(*requester);
        requester->connect(pluginSocketAdd);
        std::unique_ptr<PluginCommunication::IPluginProxy> proxyPlugin = std::unique_ptr<PluginProxy>(new PluginProxy(std::move(requester), pluginName));
        m_RegisteredPlugins[pluginName] = std::move(proxyPlugin);
    }

    std::unique_ptr<PluginCommunication::IPluginProxy>& PluginManager::getPlugin(const std::string &pluginName)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        if (m_RegisteredPlugins.find(pluginName) != m_RegisteredPlugins.end())
        {
            return (m_RegisteredPlugins[pluginName]);
        }
        throw PluginCommunication::IPluginCommunicationException("Tried to access non-registered plugin");
    }

    void PluginManager::removePlugin(const std::string &pluginName)
    {
        m_RegisteredPlugins.erase(pluginName);
    }

    void PluginManager::setTimeouts(Common::ZeroMQWrapper::ISocketSetup &socket)
    {
        socket.setTimeout(m_defaultTimeout);
        socket.setConnectionTimeout(m_defaultConnectTimeout);
    }


}
}

