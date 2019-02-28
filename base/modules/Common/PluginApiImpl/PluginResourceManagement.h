/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/ISocketSetup.h>

namespace Common
{
    namespace PluginApiImpl
    {
        class PluginResourceManagement : public virtual Common::PluginApi::IPluginResourceManagement
        {
        public:
            PluginResourceManagement();
            explicit PluginResourceManagement(Common::ZMQWrapperApi::IContextSharedPtr);

            std::unique_ptr<Common::PluginApi::IBaseServiceApi> createPluginAPI(
                const std::string& pluginName,
                std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback) override;
            std::unique_ptr<Common::PluginApi::IRawDataPublisher> createRawDataPublisher() override;
            std::unique_ptr<Common::PluginApi::ISubscriber> createSubscriber(
                const std::string& sensorDataCategorySubscription,
                std::shared_ptr<Common::PluginApi::IEventVisitorCallback> sensorDataCallback) override;

            std::unique_ptr<PluginApi::ISubscriber> createRawSubscriber(
                const std::string& dataCategorySubscription,
                std::shared_ptr<PluginApi::IRawDataCallback> rawDataCallback) override;

            /* mainly for tests */
            void setDefaultTimeout(int timeoutMs);
            void setDefaultConnectTimeout(int timeoutMs);
            Common::ZMQWrapperApi::IContextSharedPtr getSocketContext();

        private:
            void setTimeouts(Common::ZeroMQWrapper::ISocketSetup& socket);
            Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr;
            int m_defaultTimeout;
            int m_defaultConnectTimeout;
        };

    } // namespace PluginApiImpl
} // namespace Common
