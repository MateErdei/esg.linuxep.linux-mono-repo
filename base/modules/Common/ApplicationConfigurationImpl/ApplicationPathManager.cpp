//
// Created by pair on 29/06/18.
//

#include "Common/FileSystem/IFileSystem.h"
#include "ApplicationPathManager.h"
#include "IApplicationConfiguration.h"

namespace Common
{
    namespace ApplicationConfigurationImpl
    {

        std::string ApplicationPathManager::getPluginSocketAddress(const std::string &pluginName) const
        {
            return socketPath("plugins/" + pluginName + ".ipc");
        }

        std::string ApplicationPathManager::getManagementAgentSocketAddress() const
        {
            return socketPath("mcs_agent.ipc");
        }

        std::string ApplicationPathManager::getWatchdogSocketAddress() const
        {
            return socketPath("watchdog.ipc");
        }

        std::string ApplicationPathManager::sophosInstall() const
        {
            return Common::ApplicationConfiguration::applicationConfiguration().getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);
        }

        std::string ApplicationPathManager::socketPath(const std::string &relative) const
        {
            return "ipc://" + sophosInstall() + "/var/ipc/" + relative;
        }

        std::string ApplicationPathManager::getPublisherDataChannelAddress() const
        {
            return socketPath("publisherdatachannel.ipc");
        }

        std::string ApplicationPathManager::getSubscriberDataChannelAddress() const
        {
            return socketPath("subscriberdatachannel.ipc");
        }

        std::string ApplicationPathManager::getPluginRegistryPath() const
        {
            return Common::FileSystem::fileSystem()->join(sophosInstall(),"/base/pluginRegistry/");
        }

        std::string ApplicationPathManager::getVersigPath() const
        {
            return Common::FileSystem::fileSystem()->join(sophosInstall(),"/bin/versig");
        }
    }


    namespace ApplicationConfiguration
    {
        std::unique_ptr<IApplicationPathManager>& instance()
        {
            static std::unique_ptr<IApplicationPathManager>  pointer = std::unique_ptr<IApplicationPathManager>(new ApplicationConfigurationImpl::ApplicationPathManager());
            return pointer;
        }

        IApplicationPathManager& applicationPathManager()
        {
            return *instance();
        }
        /** Use only for test */
        void replaceApplicationPathManager( std::unique_ptr<IApplicationPathManager>  applicationPathManager)
        {
            instance().reset(applicationPathManager.release());
        }
        void restoreApplicationPathManager()
        {
            instance().reset(new ApplicationConfigurationImpl::ApplicationPathManager());
        }

    }

}