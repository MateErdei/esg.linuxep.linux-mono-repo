/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>
#include <memory>
#include <functional>
namespace Common
{
    namespace ApplicationConfiguration
    {
        class IApplicationPathManager
        {
        public:
            virtual ~IApplicationPathManager() = default;
            virtual std::string getPluginSocketAddress(const std::string & pluginName) const = 0;
            virtual std::string getManagementAgentSocketAddress() const = 0;
            virtual std::string getWatchdogSocketAddress() const = 0;
            virtual std::string sophosInstall() const = 0;

            /**
             * Get the directory to store root logs for base processes.
             * @return
             */
            virtual std::string getBaseLogDirectory() const = 0;

            virtual std::string getPublisherDataChannelAddress() const = 0;
            virtual std::string getSubscriberDataChannelAddress() const = 0;
            virtual std::string getPluginRegistryPath() const = 0;
            virtual std::string getVersigPath() const = 0;
            virtual std::string getMcsPolicyFilePath() const = 0;
            virtual std::string getMcsActionFilePath() const = 0;
            virtual std::string getMcsStatusFilePath() const = 0;
            virtual std::string getMcsEventFilePath() const = 0;

            virtual std::string getLocalWarehouseRepository() const = 0;

            virtual std::string getLocalDistributionRepository() const = 0;

            virtual std::string getTempPath() const = 0;
        };


        IApplicationPathManager& applicationPathManager();
        /** Use only for test */
        void replaceApplicationPathManager( std::unique_ptr<IApplicationPathManager> );
        void restoreApplicationPathManager();

    }
}


