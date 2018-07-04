//
// Created by pair on 29/06/18.
//

#ifndef EVEREST_BASE_IAPPLICATIONPATHMANAGER_H
#define EVEREST_BASE_IAPPLICATIONPATHMANAGER_H

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
            virtual std::string getPublisherDataChannelAddress() const = 0;
            virtual std::string getSubscriberDataChannelAddress() const = 0;
        };


        IApplicationPathManager& applicationPathManager();
        /** Use only for test */
        void replaceApplicationPathManager( std::unique_ptr<IApplicationPathManager> );
        void restoreApplicationPathManager();

    }
}

#endif //EVEREST_BASE_IAPPLICATIONPATHMANAGER_H
