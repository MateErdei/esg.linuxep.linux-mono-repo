/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "PluginCallBackHandler.h"

#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/PluginProtocol/MessageBuilder.h"
#include "Common/ZeroMQWrapper/ISocketReplierPtr.h"
#include "Common/ZeroMQWrapper/ISocketRequesterPtr.h"

namespace Common
{
    namespace PluginApiImpl
    {
        class BaseServiceAPI : public virtual Common::PluginApi::IBaseServiceApi
        {
        public:
            BaseServiceAPI(const std::string& pluginName, Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester);
            ~BaseServiceAPI() override;

            void setPluginCallback(
                const std::string& pluginName,
                std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback,
                Common::ZeroMQWrapper::ISocketReplierPtr);

            void sendEvent(const std::string& appId, const std::string& eventXml) const override;

            void sendStatus(
                const std::string& appId,
                const std::string& statusXml,
                const std::string& statusWithoutTimestampsXml) const override;

            void requestPolicies(const std::string& appId) const override;

            void registerWithManagementAgent() const;

        private:
            Common::PluginProtocol::DataMessage getReply(const Common::PluginProtocol::DataMessage& request) const;

            std::string m_pluginName;
            Common::ZeroMQWrapper::ISocketRequesterPtr m_socket;

            std::unique_ptr<PluginCallBackHandler> m_pluginCallbackHandler;
            Common::PluginProtocol::MessageBuilder m_messageBuilder;
        };
    } // namespace PluginApiImpl
} // namespace Common
