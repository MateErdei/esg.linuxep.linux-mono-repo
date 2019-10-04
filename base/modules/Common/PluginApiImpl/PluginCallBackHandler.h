/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/PluginProtocol/AbstractListenerServer.h"
#include "Common/PluginProtocol/MessageBuilder.h"

#include <Common/ZeroMQWrapper/IReadWrite.h>

namespace Common
{
    namespace PluginApiImpl
    {
        class PluginCallBackHandler : public Common::PluginProtocol::AbstractListenerServer
        {
        public:
            PluginCallBackHandler(
                const std::string& pluginName,
                std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback,
                Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY policy =
                    Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY::HANDLESHUTDOWN);

        private:
            Common::PluginProtocol::DataMessage process(
                const Common::PluginProtocol::DataMessage& request) const override;
            void onShutdownRequested() override;

            Common::PluginProtocol::MessageBuilder m_messageBuilder;
            std::shared_ptr<Common::PluginApi::IPluginCallbackApi> m_pluginCallback;
        };
    } // namespace PluginApiImpl
} // namespace Common
