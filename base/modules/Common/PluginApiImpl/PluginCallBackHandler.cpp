/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallBackHandler.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginCommunication/IPluginCommunicationException.h>
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

        std::string PluginCallBackHandler::GetContentFromPayload(Common::PluginProtocol::Commands commandType, const std::string& payload) const
        {
            // If the payload does not contain .xml extension assume payload is not a file name but the data
            // if the payload is the data, this should only be for commands such as TriggerUpdate from watchdog.

            std::string payloadData(payload);
            if(std::count(payload.begin(), payload.end(), '.') > 1)
            {
                throw PluginCommunication::IPluginCommunicationException("Invalid payload in message");
            }

            if ((payload.find(".xml") != std::string::npos) || (payload.find(".json") != std::string::npos))
            {
                std::string rootPath("");
                if(commandType == Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY)
                {
                    rootPath = Common::ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath();
                }
                else if (commandType == Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION)
                {
                    rootPath = Common::ApplicationConfiguration::applicationPathManager().getMcsActionFilePath();
                }
                else
                {
                    throw PluginCommunication::IPluginCommunicationException("Unable to determine payload path");
                }

                try
                {
                    payloadData = Common::FileSystem::fileSystem()->readFile(Common::FileSystem::join(rootPath, payload));
                    return payloadData;
                }
                catch(Common::FileSystem::IFileSystemException& ex)
                {
                    std::stringstream errorMessage;
                    errorMessage << "Failed to read action file" << payload << ", error, " << ex.what();
                    throw PluginCommunication::IPluginCommunicationException(errorMessage.str());
                }
            }
            else
            {
                // Allowed list of non file name strings
                for(auto& allowedStringData : {"TriggerUpdate"})
                {
                    if(payloadData == allowedStringData)
                    {
                        return payloadData;
                    }
                }
            }

            throw PluginCommunication::IPluginCommunicationException("Invalid command sent.");
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

                        // Some actions are passed as content when not comming from MCS communication channel.
                        // i.e. when comming directly from watchdog

                        m_pluginCallback->applyNewPolicy(
                            GetContentFromPayload(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY,
                                                  m_messageBuilder.requestExtractPolicy(request)));

                        return m_messageBuilder.replyAckMessage(request);
                    case Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION:
                        LOGSUPPORT("Received new Action");

                        m_pluginCallback->queueActionWithCorrelation(
                            GetContentFromPayload(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION,
                                                  m_messageBuilder.requestExtractAction(request)), request.m_correlationId);
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