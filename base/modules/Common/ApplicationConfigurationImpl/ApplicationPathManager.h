//
// Created by pair on 29/06/18.
//

#ifndef EVEREST_BASE_APPLICATIONPATHMANAGER_H
#define EVEREST_BASE_APPLICATIONPATHMANAGER_H
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
namespace Common
{
    namespace ApplicationConfigurationImpl
    {
        class ApplicationPathManager : public virtual Common::ApplicationConfiguration::IApplicationPathManager
        {
        public:
            std::string getPluginSocketAddress(const std::string & pluginName) const override ;
            std::string getManagementAgentSocketAddress() const override ;
            std::string getWatchdogSocketAddress() const override ;
            std::string sophosInstall() const override ;
            std::string getPublisherDataChannelAddress() const override ;
            std::string getSubscriberDataChannelAddress() const override ;
            std::string getPluginRegistryPath() const override;
        private:
            std::string socketPath( const std::string & relative) const;
        };

    }
}



#endif //EVEREST_BASE_APPLICATIONPATHMANAGER_H
