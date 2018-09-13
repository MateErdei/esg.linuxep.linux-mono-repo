/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseServiceAPI.h"
#include "Logger.h"
#include <Common/PluginApi/ApiException.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

Common::PluginApiImpl::BaseServiceAPI::BaseServiceAPI(const std::string &pluginName,
                                                    Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester)
        : m_pluginName(pluginName), m_socket(std::move(socketRequester)), m_pluginCallbackHandler(),
          m_messageBuilder(Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion, pluginName)
{
    LOGSUPPORT("Plugin initialized: " << pluginName);
    LOGSUPPORT("Uses protocol version :" << Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion);
}

void Common::PluginApiImpl::BaseServiceAPI::setPluginCallback(const std::string &pluginName,
                                                             std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback,
                                                             Common::ZeroMQWrapper::ISocketReplierPtr replier)
{
    m_pluginCallbackHandler.reset(new PluginCallBackHandler(pluginName, std::move(replier), std::move(pluginCallback)));
    m_pluginCallbackHandler->start();
}

Common::PluginApiImpl::BaseServiceAPI::~BaseServiceAPI()
{
    if( m_pluginCallbackHandler)
    {
        m_pluginCallbackHandler->stop();
    }
}



void Common::PluginApiImpl::BaseServiceAPI::sendEvent(const std::string &appId, const std::string &eventXml) const
{
    LOGSUPPORT("Send Event for AppId: " << appId);


    Common::PluginProtocol::DataMessage replyMessage = getReply(
            m_messageBuilder.requestSendEventMessage(appId, eventXml)
    );

    LOGSUPPORT("Received Send Event reply from management agent for AppId: " << appId);

    if (!m_messageBuilder.hasAck(replyMessage))
    {
        std::string errorMessage("Invalid reply for: 'send event'");
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}

void Common::PluginApiImpl::BaseServiceAPI::sendStatus(const std::string &appId, const std::string &statusXml,
                                                      const std::string &statusWithoutTimestampsXml) const
{
    LOGSUPPORT("Change status message for AppId: " << appId);

    Common::PluginProtocol::DataMessage replyMessage = getReply(
            m_messageBuilder.requestSendStatusMessage(appId, statusXml, statusWithoutTimestampsXml)
    );


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

    Common::PluginProtocol::DataMessage replyMessage = getReply(
            m_messageBuilder.requestRegisterMessage()
    );
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

    Common::PluginProtocol::DataMessage reply = getReply(
            m_messageBuilder.requestCurrentPolicyMessage(appId)
    );

    LOGSUPPORT("Received policy from management agent for AppId: " << appId);
    if (!m_messageBuilder.hasAck(reply))
    {

        std::string errorMessage = "Request policies failed with error: ";
        errorMessage += reply.Error;
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}


Common::PluginProtocol::DataMessage Common::PluginApiImpl::BaseServiceAPI::getReply(const Common::PluginProtocol::DataMessage &request) const
{
    Common::PluginProtocol::Protocol protocol;
    Common::PluginProtocol::DataMessage reply;
    try
    {
        m_socket->write(protocol.serialize(request));
        reply = protocol.deserialize(m_socket->read());
    }
    catch ( std::exception & ex)
    {
        throw Common::PluginApi::ApiException(ex.what());
    }

    if( reply.Command != request.Command)
    {
        std::string errorMessage("Received reply from wrong command, expecting" + Common::PluginProtocol::SerializeCommand(request.Command) +
                                 ", Received: " + Common::PluginProtocol::SerializeCommand(reply.Command));
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }

    if ( !reply.Error.empty())
    {
        std::string errorMessage("Invalid reply, error: " + reply.Error);
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }

    return reply;

}

const std::string Common::PluginApi::getInstallRoot()
{
    return Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
}