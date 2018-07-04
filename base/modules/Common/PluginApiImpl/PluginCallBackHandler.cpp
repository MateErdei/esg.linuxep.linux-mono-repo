/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/PluginApi/ApiException.h"
#include "PluginCallBackHandler.h"
#include "ProtocolSerializerFactory.h"
namespace Common
{
    namespace PluginApiImpl
    {

        PluginCallBackHandler::PluginCallBackHandler(std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                                                     std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback):
                 AbstractListenerServer(std::move(ireadWrite))
                , m_messageBuilder("NotUsed", ProtocolSerializerFactory::ProtocolVersion)
                , m_pluginCallback(pluginCallback)
        {
        }

        DataMessage PluginCallBackHandler::process(const DataMessage &request) const
        {
            try
            {

                switch (request.Command)
                {
                    case Commands::REQUEST_PLUGIN_APPLY_POLICY:
                        m_pluginCallback->applyNewPolicy(m_messageBuilder.requestExtractPolicy(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Commands::REQUEST_PLUGIN_DO_ACTION:
                        m_pluginCallback->doAction(m_messageBuilder.requestExtractAction(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Commands::REQUEST_PLUGIN_STATUS:
                        {
                            StatusInfo statusInfo = m_pluginCallback->getStatus();
                            return m_messageBuilder.replyStatus(request, statusInfo);
                        }
                    case Commands::REQUEST_PLUGIN_TELEMETRY:
                        return m_messageBuilder.replyTelemetry(request, m_pluginCallback->getTelemetry());
                    default:
                        return m_messageBuilder.replySetErrorIfEmpty(request, "Request not supported");

                }
            }
            catch ( Common::PluginApi::ApiException & ex)
            {
                return m_messageBuilder.replySetErrorIfEmpty(request, ex.what());
            }
            catch ( std::exception & ex)
            {
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