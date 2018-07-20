/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "ApplicationConfiguration/IApplicationPathManager.h"
#include "IPluginCommunicationException.h"
#include "PluginManager.h"
#include "PluginProxy.h"
#include "PluginServerCallback.h"
#include "Logger.h"
#include <thread>

namespace ManagementAgent
{
namespace PluginCommunicationImpl
{
    PluginManager::PluginManager() :
            m_context(Common::ZeroMQWrapper::createContext()), m_serverCallbackHandler(), m_defaultTimeout(5000), m_defaultConnectTimeout(5000)
    {
        auto replier = m_context->getReplier();
        setTimeouts(*replier);
        std::string managementSocketAdd = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        replier->listen(managementSocketAdd);
        std::shared_ptr<PluginServerCallback> serverCallback = std::make_shared<PluginServerCallback>(*this);
        m_serverCallbackHandler.reset( new PluginServerCallbackHandler( std::move(replier), serverCallback));
        m_serverCallbackHandler->start();
    }

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
        if( m_serverCallbackHandler)
        {
            m_serverCallbackHandler->stop();
        }
        m_serverCallbackHandler.reset( new PluginServerCallbackHandler( std::move(replier), std::move(serverCallback) ) );
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


    int PluginManager::applyNewPolicy(const std::string &appId, const std::string &policyXml)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        int pluginsNotified = 0;
        for (auto &proxy : m_RegisteredPlugins)
        {
            if (proxy.second->hasPolicyAppId(appId))
            {
                try
                {
                    proxy.second->applyNewPolicy(appId, policyXml);
                    ++pluginsNotified;
                }catch ( std::exception & ex)
                {
                    LOGERROR("Failure on sending policy to " << proxy.first << ". Reason: " << ex.what());
                }
            }
        }
        return pluginsNotified;
    }

    int PluginManager::queueAction(const std::string &appId, const std::string &actionXml)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        int pluginsNotified = 0;
        for (auto &proxy : m_RegisteredPlugins)
        {
            if (proxy.second->hasActionAppId(appId))
            {
                try
                {
                    proxy.second->queueAction(appId, actionXml);
                    ++pluginsNotified;
                }catch ( std::exception & ex)
                {
                    LOGERROR("Failure on sending action to " << proxy.first << ". Reason: " << ex.what());
                }
            }
        }
        return pluginsNotified;
    }

    std::vector<Common::PluginApi::StatusInfo> PluginManager::getStatus(const std::string & pluginName)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        return getPlugin(pluginName)->getStatus();
    }

    std::string PluginManager::getTelemetry(const std::string & pluginName)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        return getPlugin(pluginName)->getTelemetry();
    }

    void PluginManager::setAppIds(const std::string &pluginName, const std::vector<std::string> &policyAppIds, const std::vector<std::string> & statusAppIds)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        auto plugin = getPlugin(pluginName);
        plugin->setPolicyAndActionsAppIds(policyAppIds);
        plugin->setStatusAppIds(statusAppIds);
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

    PluginCommunication::IPluginProxy* PluginManager::getPlugin(const std::string &pluginName)
    {
        auto found =m_RegisteredPlugins.find(pluginName);
        if ( found != m_RegisteredPlugins.end())
        {
            return found->second.get();
        }
        throw PluginCommunication::IPluginCommunicationException("Tried to access non-registered plugin");
    }

    void PluginManager::removePlugin(const std::string &pluginName)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        m_RegisteredPlugins.erase(pluginName);
    }

    Common::ZeroMQWrapper::IContext& PluginManager::getSocketContext()
    {
        return *m_context;
    }

    void PluginManager::setTimeouts(Common::ZeroMQWrapper::ISocketSetup &socket)
    {
        socket.setTimeout(m_defaultTimeout);
        socket.setConnectionTimeout(m_defaultConnectTimeout);
    }

    void PluginManager::setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver)
    {
//        if (m_serverCallbackHandler != nullptr)
//        {
//            PluginServerCallback* test = dynamic_cast<PluginServerCallback>(m_serverCallbackHandler.get());
//            if (test != nullptr)
//            {
//                test->setStatusReceiver(statusReceiver);
//            }
//        }
    }
}
}

