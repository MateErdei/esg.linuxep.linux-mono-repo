/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PLUGINRESOURCEMANAGEMENT_H
#define EVEREST_BASE_PLUGINRESOURCEMANAGEMENT_H
#include "IPluginResourceManagement.h"
#include "Common/ZeroMQWrapper/IContextPtr.h"
#include "Common/ZeroMQWrapper/ISocketSetup.h"
namespace Common
{
    namespace PluginApiImpl
    {
        class PluginResourceManagement : public virtual Common::PluginApi::IPluginResourceManagement
        {
        public:
            PluginResourceManagement();

            void setDefaultTimeout(int timeoutMs) override ;
            void setDefaultConnectTimeout(int timeoutMs) override ;
            std::unique_ptr<Common::PluginApi::IPluginApi> createPluginAPI( const std::string & pluginName, std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback)  override ;
            std::unique_ptr<Common::PluginApi::ISensorDataPublisher> createSensorDataPublisher(const std::string & pluginName) override ;
            std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> createSensorDataSubscriber(const std::string & sensorDataCategorySubscription,
                                                                                                 std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback) override ;

            /* mainly for tests */
            Common::ZeroMQWrapper::IContext & getSocketContext();
        private:
            void setTimeouts( Common::ZeroMQWrapper::ISocketSetup & socket);
            Common::ZeroMQWrapper::IContextPtr m_context;
            int m_defaulTimeout;
            int m_defaultConnectTimeout;
        };

    }
}


#endif //EVEREST_BASE_PLUGINRESOURCEMANAGEMENT_H
