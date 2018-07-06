/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/PluginApi/ApiException.h>
#include "PluginApiImpl.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "SharedSocketContext.h"

#include <iostream>

#define LOGERROR(x) std::cerr << x << '\n'
#define LOGDEBUG(x) std::cerr << x << '\n'
#define LOGSUPPORT(x) std::cerr << x << '\n'

Common::PluginApiImpl::PluginApiImpl::PluginApiImpl(const std::string &pluginName,
                                                    Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester)
: m_pluginName(pluginName), m_socket(std::move(socketRequester)), m_pluginCallbackHandler()
{

}

void Common::PluginApiImpl::PluginApiImpl::setPluginCallback(
        std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback, Common::ZeroMQWrapper::ISocketReplierPtr replier)
{
    m_pluginCallbackHandler.reset( new PluginCallBackHandler( std::move(replier), pluginCallback) );
    m_pluginCallbackHandler->start();
}

Common::PluginApiImpl::PluginApiImpl::~PluginApiImpl()
{
    if( m_pluginCallbackHandler)
    {
        m_pluginCallbackHandler->stop();
    }
}



void Common::PluginApiImpl::PluginApiImpl::sendEvent(const std::string &appId, const std::string &eventXml) const
{
    Common::PluginProtocol::MessageBuilder messageBuilder(appId, Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion);
    Common::PluginProtocol::DataMessage message = messageBuilder.requestSendEventMessage(eventXml);
    Common::PluginProtocol::DataMessage replyMessage = getReply(message);
    if ( !messageBuilder.hasAck(replyMessage))
    {
        std::string errorMessage("Invalid reply for: 'send event'");
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}

void Common::PluginApiImpl::PluginApiImpl::changeStatus(const std::string &appId, const std::string &statusXml,
                                                        const std::string &statusWithoutTimestampsXml) const
{
    Common::PluginProtocol::MessageBuilder messageBuilder(appId, Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion);
    Common::PluginProtocol::DataMessage message = messageBuilder.requestSendStatusMessage(statusXml, statusWithoutTimestampsXml);

    Common::PluginProtocol::DataMessage replyMessage = getReply(message);
    if ( !messageBuilder.hasAck(replyMessage))
    {
        std::string errorMessage("Invalid reply for: 'Change status'");
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}

void Common::PluginApiImpl::PluginApiImpl::registerWithManagementAgent() const
{
    Common::PluginProtocol::MessageBuilder messageBuilder(m_pluginName, Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion);
    Common::PluginProtocol::DataMessage message = messageBuilder.requestRegisterMessage();

    Common::PluginProtocol::DataMessage replyMessage = getReply(message);
    if ( !messageBuilder.hasAck(replyMessage))
    {
        std::string errorMessage("Invalid reply for: 'Registration'");
        LOGERROR(errorMessage);
        throw Common::PluginApi::ApiException(errorMessage);
    }
}



std::string Common::PluginApiImpl::PluginApiImpl::getPolicy(const std::string &appId) const
{
    Common::PluginProtocol::MessageBuilder messageBuilder(appId, Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion);
    Common::PluginProtocol::DataMessage message = messageBuilder.requestCurrentPolicyMessage();
    Common::PluginProtocol::DataMessage  reply = getReply(message);

    return messageBuilder.replyExtractCurrentPolicy(reply);
}


Common::PluginProtocol::DataMessage Common::PluginApiImpl::PluginApiImpl::getReply(const Common::PluginProtocol::DataMessage &request) const
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
                                 ", Received: " + Common::PluginProtocol::SerializeCommand(request.Command));
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



