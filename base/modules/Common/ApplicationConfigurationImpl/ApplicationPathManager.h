/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

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
            std::string getVersigPath() const override ;
        private:
            std::string socketPath( const std::string & relative) const;
        };

    }
}




