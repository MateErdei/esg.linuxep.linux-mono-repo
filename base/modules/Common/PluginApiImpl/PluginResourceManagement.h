/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/ZeroMQWrapper/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/ISocketSetup.h>

namespace Common
{
    namespace PluginApiImpl
    {
        class PluginResourceManagement : public virtual Common::PluginApi::IPluginResourceManagement
        {
        public:
            PluginResourceManagement();
            explicit PluginResourceManagement(Common::ZeroMQWrapper::IContextSharedPtr);

            std::unique_ptr<Common::PluginApi::IBaseServiceApi> createPluginAPI( const std::string & pluginName, std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback)  override ;
            std::unique_ptr<Common::PluginApi::ISensorDataPublisher> createSensorDataPublisher(const std::string & pluginName) override ;
            std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> createSensorDataSubscriber(const std::string & sensorDataCategorySubscription,
                                                                                                 std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback) override ;

            /* mainly for tests */
            void setDefaultTimeout(int timeoutMs) ;
            void setDefaultConnectTimeout(int timeoutMs) ;
            Common::ZeroMQWrapper::IContextSharedPtr getSocketContext();
        private:
            void setTimeouts( Common::ZeroMQWrapper::ISocketSetup & socket);
            Common::ZeroMQWrapper::IContextSharedPtr m_contextPtr;
            int m_defaulTimeout;
            int m_defaultConnectTimeout;
        };

    }
}



