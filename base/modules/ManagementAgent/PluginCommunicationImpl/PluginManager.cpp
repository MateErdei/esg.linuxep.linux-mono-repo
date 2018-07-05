//
// Created by pair on 05/07/18.
//

#include "ApplicationConfiguration/IApplicationPathManager.h"
#include "PluginManager.h"

namespace ManagementAgent
{
namespace PluginCommunicationImpl
{
    PluginManager::PluginManager() :
    m_serverCallbackHandler()
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

    void PluginManager::registerPlugin(std::string pluginName)
    {
        std::string pluginSocketAdd = Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(pluginName);

    }

    std::shared_ptr<PluginCommunication::IPluginProxy> PluginManager::getPlugin(std::string pluginName)
    {

    }

    void PluginManager::removePlugin(std::string pluginName)
    {

    }

}
}

