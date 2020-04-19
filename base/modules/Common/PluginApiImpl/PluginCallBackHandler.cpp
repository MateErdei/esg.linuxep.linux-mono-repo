/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallBackHandler.h"

#include "Logger.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

namespace Common
{
    namespace PluginApiImpl
    {
        PluginCallBackHandler::PluginCallBackHandler(
            const std::string& pluginName,
            std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
            std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback,
            Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY policy) :
            AbstractListenerServer(std::move(ireadWrite), policy),
            m_messageBuilder(pluginName),
            m_pluginCallback(std::move(pluginCallback))
        {
        }

        Common::PluginProtocol::DataMessage PluginCallBackHandler::process(
            const Common::PluginProtocol::DataMessage& request) const
        {
            try
            {
                switch (request.m_command)
                {
                    case Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY:
                        LOGSUPPORT("Received new policy");
                        m_pluginCallback->applyNewPolicy(m_messageBuilder.requestExtractPolicy(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION:
                        LOGSUPPORT("Received new Action");
                        m_pluginCallback->queueActionWithCorrelation(m_messageBuilder.requestExtractAction(request), request.m_correlationId);
                        return m_messageBuilder.replyAckMessage(request);
                    case Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS:
                    {
                        LOGSUPPORT("Received request for current status");
                        Common::PluginApi::StatusInfo statusInfo = m_pluginCallback->getStatus(request.m_applicationId);
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
            catch (Common::PluginApi::ApiException& ex)
            {
                LOGERROR("ApiException: Failed to process received request: " << ex.what());
                return m_messageBuilder.replySetErrorIfEmpty(request, ex.what());
            }
            catch (std::exception& ex)
            {
                LOGERROR("Unexpected error, failed to process received request: " << ex.what());
                return m_messageBuilder.replySetErrorIfEmpty(request, ex.what());
            }
        }

        void PluginCallBackHandler::onShutdownRequested()
        {
            LOGSUPPORT("Saving plugin telemetry before shutdown");
            Common::Telemetry::TelemetryHelper::getInstance().save();
            m_pluginCallback->onShutdown();
            stop();
        }
    } // namespace PluginApiImpl

} // namespace Common