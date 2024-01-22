// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "PluginCallBackHandler.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginCommunication/IPluginCommunicationException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <thread>

namespace Common::PluginApiImpl
{
    PluginCallBackHandler::PluginCallBackHandler(
        const std::string& pluginName,
        std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
        std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback,
        Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY policy) :
        AbstractListenerServer(std::move(ireadWrite), policy),
        m_messageBuilder(pluginName),
        pluginName_(pluginName),
        m_pluginCallback(std::move(pluginCallback))
    {
    }

    PluginCallBackHandler::~PluginCallBackHandler()
    {
        if (!m_shutdownRequested)
        {
            LOGDEBUG("Saving plugin telemetry before shutdown");
            Common::Telemetry::TelemetryHelper::getInstance().save(pluginName_);
        }
    }

    std::string PluginCallBackHandler::GetContentFromPayload(
        Common::PluginProtocol::Commands commandType,
        const std::string& payload) const
    {
        // If the payload does not contain .xml extension assume payload is not a file name but the data.
        // If the payload is the data, this should only be for commands such as TriggerUpdate from watchdog.

        std::string payloadData(payload);
        if (Common::FileSystem::basename(payload) != payload)
        {
            LOGINFO(payload);
            throw PluginCommunication::IPluginCommunicationException(
                "Invalid payload in message, expected only file name");
        }

        if ((payload.find(".xml") != std::string::npos) || (payload.find(".json") != std::string::npos))
        {
            std::string rootPath;
            if (commandType == Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY)
            {
                if (payload.find("internal") != std::string::npos)
                {
                    rootPath = Common::ApplicationConfiguration::applicationPathManager().getInternalPolicyFilePath();
                }
                else
                {
                    rootPath = Common::ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath();
                }
            }
            else if (commandType == Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION)
            {
                rootPath = Common::ApplicationConfiguration::applicationPathManager().getMcsActionFilePath();
            }
            else
            {
                throw PluginCommunication::IPluginCommunicationException("Unable to determine payload path");
            }

            // Add some extra resilience here to retry reading the policy file as we've seen some very occasional
            // cases of the file failing to be read. https://sophos.atlassian.net/browse/LINUXDAR-5816
            int tries = 0;
            int maxTries = 3;
            while (tries <= maxTries)
            {
                ++tries;
                try
                {
                    payloadData =
                        Common::FileSystem::fileSystem()->readFile(Common::FileSystem::join(rootPath, payload));
                    return payloadData;
                }
                catch (Common::FileSystem::IFileSystemException& ex)
                {
                    if (tries <= maxTries)
                    {
                        LOGDEBUG("Failed to read MCS file " << payload << ", will retry. Exception: " << ex.what());
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    }
                    else
                    {
                        LOGDEBUG("Failed to read MCS file " << payload << ". Exception: " << ex.what());
                    }
                }
            }

            // If we got here then we failed to read the file, even after retrying.
            std::stringstream errorMessage;
            errorMessage << "Failed to read MCS file: " << payload;
            throw PluginCommunication::IPluginCommunicationException(errorMessage.str());
        }
        else
        {
            // Allowed list of non file name strings
            for (auto& allowedStringData : { "TriggerUpdate", "TriggerDiagnose" })
            {
                if (payloadData == allowedStringData)
                {
                    return payloadData;
                }
            }
        }
        LOGDEBUG(payload);

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
                    LOGDEBUG("Received new policy");

                    // Some actions are passed as content when not coming from MCS communication channel.
                    // i.e. when coming directly from watchdog

                    m_pluginCallback->applyNewPolicyWithAppId(
                        request.m_applicationId,
                        GetContentFromPayload(
                            Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY,
                            m_messageBuilder.requestExtractPolicy(request)));

                    return m_messageBuilder.replyAckMessage(request);
                case Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION:
                    LOGDEBUG("Received new Action: " << request.m_correlationId);

                    m_pluginCallback->queueActionWithCorrelation(
                        GetContentFromPayload(
                            Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION,
                            m_messageBuilder.requestExtractAction(request)),
                        request.m_correlationId);
                    return m_messageBuilder.replyAckMessage(request);
                case Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS:
                {
                    LOGDEBUG("Received request for current status");
                    Common::PluginApi::StatusInfo statusInfo = m_pluginCallback->getStatus(request.m_applicationId);
                    return m_messageBuilder.replyStatus(request, statusInfo);
                }
                case Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY:
                    LOGDEBUG("Received request for current telemetry.");
                    return m_messageBuilder.replyTelemetry(request, m_pluginCallback->getTelemetry());
                case Common::PluginProtocol::Commands::REQUEST_PLUGIN_HEALTH:
                    LOGDEBUG("Received request for current health.");
                    return m_messageBuilder.replyHealth(request, m_pluginCallback->getHealth());
                default:
                    LOGDEBUG("Received invalid request.");
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
        LOGDEBUG("Saving plugin telemetry before shutdown");
        Common::Telemetry::TelemetryHelper::getInstance().save(pluginName_);
        m_shutdownRequested = true;

        m_pluginCallback->onShutdown();
        stop();
    }
} // namespace Common::PluginApiImpl