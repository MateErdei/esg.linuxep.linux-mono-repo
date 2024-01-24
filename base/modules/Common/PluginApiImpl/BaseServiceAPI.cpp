// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "BaseServiceAPI.h"

#include "Logger.h"

#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApi/NoACKReplyException.h"
#include "Common/PluginApi/NoPolicyAvailableException.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"

#include <thread>

Common::PluginApiImpl::BaseServiceAPI::BaseServiceAPI(
    const std::string& pluginName,
    Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester) :
    m_pluginName(pluginName),
    m_socket(std::move(socketRequester)),
    m_pluginCallbackHandler(),
    m_messageBuilder(pluginName)
{
    LOGSUPPORT("Plugin initialized: " << pluginName);
}

void Common::PluginApiImpl::BaseServiceAPI::setPluginCallback(
    const std::string& pluginName,
    std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback,
    Common::ZeroMQWrapper::ISocketReplierPtr replier)
{
    m_pluginCallbackHandler.reset(new PluginCallBackHandler(pluginName, std::move(replier), std::move(pluginCallback)));
    m_pluginCallbackHandler->start();
}

Common::PluginApiImpl::BaseServiceAPI::~BaseServiceAPI()
{
    if (m_pluginCallbackHandler)
    {
        m_pluginCallbackHandler->stopAndJoin();
    }
}

void Common::PluginApiImpl::BaseServiceAPI::sendEvent(const std::string& appId, const std::string& eventXml) const
{
    LOGSUPPORT("Send Event for AppId: " << appId);

    Common::PluginProtocol::DataMessage replyMessage =
        getReply(m_messageBuilder.requestSendEventMessage(appId, eventXml));

    LOGSUPPORT("Received Send Event reply from management agent for AppId: " << appId);

    if (!m_messageBuilder.hasAck(replyMessage))
    {
        std::string errorMessage("Invalid reply for: 'send event'");
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}

void Common::PluginApiImpl::BaseServiceAPI::sendStatus(
    const std::string& appId,
    const std::string& statusXml,
    const std::string& statusWithoutTimestampsXml) const
{
    LOGSUPPORT("Change status message for AppId: " << appId);

    Common::PluginProtocol::DataMessage replyMessage =
        getReply(m_messageBuilder.requestSendStatusMessage(appId, statusXml, statusWithoutTimestampsXml));

    LOGSUPPORT("Received Change status reply from management agent for AppId: " << appId);

    if (!m_messageBuilder.hasAck(replyMessage))
    {
        std::string errorMessage("Invalid reply for: 'Change status'");
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}

void Common::PluginApiImpl::BaseServiceAPI::registerWithManagementAgent() const
{
    LOGSUPPORT("Registering '" << m_pluginName << "' with management agent");

    Common::PluginProtocol::DataMessage replyMessage = getReply(m_messageBuilder.requestRegisterMessage());
    if (!m_messageBuilder.hasAck(replyMessage))
    {
        std::string errorMessage("Invalid reply for: 'Registration'");
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}

void Common::PluginApiImpl::BaseServiceAPI::requestPolicies(const std::string& appId) const
{
    LOGSUPPORT("Request policy message for AppId: " << appId);
    Common::PluginProtocol::DataMessage reply = getReply(m_messageBuilder.requestCurrentPolicyMessage(appId));
    if (!m_messageBuilder.hasAck(reply))
    {
        throw Common::PluginApi::NoACKReplyException("No ack received for the request of policy for appId: " + appId);
    }

    LOGSUPPORT("Received policy from management agent for AppId: " << appId);
}

Common::PluginProtocol::DataMessage Common::PluginApiImpl::BaseServiceAPI::getReply(
    const Common::PluginProtocol::DataMessage& request) const
{
    static std::mutex mutex;
    std::lock_guard<std::mutex> lk(mutex);

    Common::PluginProtocol::Protocol protocol;
    Common::PluginProtocol::DataMessage reply;

    int attempts = 0;
    int tries = 2;
    int waitTimeMillis = 200;
    auto start = std::chrono::steady_clock::now();
    int totalWaitMs = 0;
    while (tries > 0)
    {
        try
        {
            attempts++;
            m_socket->write(protocol.serialize(request));
            reply = protocol.deserialize(m_socket->read());
            break;
        }
        catch (const std::exception& ex)
        {
            tries--;
            if (tries == 0)
            {
                auto end = std::chrono::steady_clock::now();
                auto duration = end - start;
                std::ostringstream err;
                err << ex.what() << " from getReply in BaseServiceAPI after " << attempts << " attempts and "
                    << totalWaitMs << "ms sleep over "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms duration";
                LOGERROR("Timed out: " << err.str());
                std::throw_with_nested(Common::PluginApi::ApiException(LOCATION, err.str()));
            }
            else
            {
                LOGDEBUG("BaseServiceAPI call failed, retrying in " << waitTimeMillis << "ms");
                std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeMillis));
                totalWaitMs += waitTimeMillis;
            }
        }
    }

    if (reply.m_command != request.m_command)
    {
        std::string errorMessage(
            "Received reply from wrong command, Expecting: " +
            Common::PluginProtocol::ConvertCommandEnumToString(request.m_command) +
            ", Received: " + Common::PluginProtocol::ConvertCommandEnumToString(reply.m_command));
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }

    if (!reply.m_error.empty())
    {
        if (reply.m_error == Common::PluginApi::NoPolicyAvailableException::NoPolicyAvailable)
        {
            throw Common::PluginApi::NoPolicyAvailableException();
        }
        std::string errorMessage("Invalid reply, error: " + reply.m_error);
        LOGSUPPORT(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }

    return reply;
}

void Common::PluginApiImpl::BaseServiceAPI::sendThreatHealth(const std::string& healthJson) const
{
    LOGDEBUG("Sending Threat Health message for plugin: " << m_pluginName << ", " << healthJson);
    Common::PluginProtocol::DataMessage replyMessage = getReply(m_messageBuilder.sendThreatHealthMessage(healthJson));

    if (!m_messageBuilder.hasAck(replyMessage))
    {
        std::string errorMessage("Invalid reply for: Threat Health message");
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}
