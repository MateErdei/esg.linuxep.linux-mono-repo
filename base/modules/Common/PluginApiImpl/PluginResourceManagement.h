/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PLUGINRESOURCEMANAGEMENT_H
#define EVEREST_BASE_PLUGINRESOURCEMANAGEMENT_H
#include "Common/PluginApi/IPluginResourceManagement.h"
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
            PluginResourceManagement(Common::ZeroMQWrapper::IContext*);

            std::unique_ptr<Common::PluginApi::IBaseServiceApi> createPluginAPI( const std::string & pluginName, std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback)  override ;
            std::unique_ptr<Common::PluginApi::ISensorDataPublisher> createSensorDataPublisher(const std::string & pluginName) override ;
            std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> createSensorDataSubscriber(const std::string & sensorDataCategorySubscription,
                                                                                                 std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback) override ;

            /* mainly for tests */
            void setDefaultTimeout(int timeoutMs) ;
            void setDefaultConnectTimeout(int timeoutMs) ;
            Common::ZeroMQWrapper::IContext & getSocketContext();
        private:
            void setTimeouts( Common::ZeroMQWrapper::ISocketSetup & socket);
            Common::ZeroMQWrapper::IContextPtr m_contextPtr;
            Common::ZeroMQWrapper::IContext* m_context;
            int m_defaulTimeout;
            int m_defaultConnectTimeout;
        };

    }
}


#endif //EVEREST_BASE_PLUGINRESOURCEMANAGEMENT_H
