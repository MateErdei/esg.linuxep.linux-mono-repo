/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/PluginApi/ApiException.h"
#include "PluginCallBackHandler.h"
#include "Common/PluginProtocol/ProtocolSerializerFactory.h"
#include "Common/PluginProtocol/Logger.h"

namespace Common
{
    namespace PluginApiImpl
    {

        PluginCallBackHandler::PluginCallBackHandler(std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                                                     std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback):
                 AbstractListenerServer(std::move(ireadWrite))
                , m_messageBuilder("NotUsed", Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion)
                , m_pluginCallback(pluginCallback)
        {
        }

        Common::PluginProtocol::DataMessage PluginCallBackHandler::process(const Common::PluginProtocol::DataMessage &request) const
        {
            try
            {

                switch (request.Command)
                {
                    case Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY:
                        LOGSUPPORT("Received new policy");
                        m_pluginCallback->applyNewPolicy(m_messageBuilder.requestExtractPolicy(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION:
                        LOGSUPPORT("Received new Action");
                        m_pluginCallback->doAction(m_messageBuilder.requestExtractAction(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS:
                        {
                            LOGSUPPORT("Received request for current status");
                            Common::PluginApi::StatusInfo statusInfo = m_pluginCallback->getStatus();
                            return m_messageBuilder.replyStatus(request, statusInfo);
                        }
                    case Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY:
                        LOGSUPPORT("Received for current telemetry.");
                        return m_messageBuilder.replyTelemetry(request, m_pluginCallback->getTelemetry());
                    default:
                        LOGSUPPORT("Received invalid request.");
                        return m_messageBuilder.replySetErrorIfEmpty(request, "Request not supported");

                }
            }
            catch ( Common::PluginApi::ApiException & ex)
            {
                LOGERROR("ApiException: Failed to process received request: " << ex.what());
                return m_messageBuilder.replySetErrorIfEmpty(request, ex.what());
            }
            catch ( std::exception & ex)
            {
                LOGERROR("Unexpected error, failed to process received request: " << ex.what());
                return m_messageBuilder.replySetErrorIfEmpty(request, ex.what());
            }

        }

        void PluginCallBackHandler::onShutdownRequested()
        {
            m_pluginCallback->shutdown();
            stop();
        }
    }

};