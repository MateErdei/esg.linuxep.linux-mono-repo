//
// Created by pair on 04/07/18.
//

#ifndef EVEREST_BASE_IPLUGINRESOURCEMANAGEMENT_H
#define EVEREST_BASE_IPLUGINRESOURCEMANAGEMENT_H
#include "IPluginApi.h"
#include "ISensorDataPublisher.h"
#include "ISensorDataSubscriber.h"

#include <memory>

namespace Common
{
    namespace PluginApi
    {
    class IPluginResourceManagement
    {
    public:
        virtual ~IPluginResourceManagement() = default;
        virtual void setDefaultTimeout(int timeoutMs) = 0;
        virtual void setDefaultConnectTimeout(int timeoutMs) = 0 ;
        virtual std::unique_ptr<IPluginApi> createPluginAPI( const std::string & pluginName, std::shared_ptr<IPluginCallback> pluginCallback)  = 0;
        virtual std::unique_ptr<ISensorDataPublisher> createSensorDataPublisher(const std::string & pluginName) = 0;
        virtual std::unique_ptr<ISensorDataSubscriber> createSensorDataSubscriber(const std::string & sensorDataCategorySubscription,
                                                                          std::shared_ptr<ISensorDataCallback> sensorDataCallback) = 0;
    };

    }
}
#endif //EVEREST_BASE_IPLUGINRESOURCEMANAGEMENT_H
