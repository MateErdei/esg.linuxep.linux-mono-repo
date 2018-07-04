//
// Created by pair on 02/07/18.
//

#include <Common/PluginApi/ApiException.h>
#include "PluginApiImpl.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "SharedSocketContext.h"

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
    MessageBuilder messageBuilder(appId, ProtocolSerializerFactory::ProtocolVersion);
    DataMessage message = messageBuilder.requestSendEventMessage(eventXml);
    getReply(message);
}

void Common::PluginApiImpl::PluginApiImpl::changeStatus(const std::string &appId, const std::string &statusXml,
                                                        const std::string &statusWithoutTimestampsXml) const
{
    MessageBuilder messageBuilder(appId, ProtocolSerializerFactory::ProtocolVersion);
    DataMessage message = messageBuilder.requestSendStatusMessage(statusXml, statusWithoutTimestampsXml);
    getReply(message);

}

std::string Common::PluginApiImpl::PluginApiImpl::getPolicy(const std::string &appId) const
{
    MessageBuilder messageBuilder(appId, ProtocolSerializerFactory::ProtocolVersion);
    DataMessage message = messageBuilder.requestCurrentPolicyMessage();
    DataMessage  reply = getReply(message);

    return messageBuilder.replyExtractCurrentPolicy(reply);
}


Common::PluginApiImpl::DataMessage Common::PluginApiImpl::PluginApiImpl::getReply(const Common::PluginApiImpl::DataMessage &request) const
{
    Protocol protocol;
    DataMessage reply;
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
        throw ApiException(
                "Received reply from wrong command, expecting" + Common::PluginApi::SerializeCommand(request.Command) +
                ", Received: " + Common::PluginApi::SerializeCommand(request.Command));
    }

    if ( !reply.Error.empty())
    {
        throw Common::PluginApi::ApiException(reply.Error);
    }
    return reply;

}


