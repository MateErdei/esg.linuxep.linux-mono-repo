//
// Created by pair on 02/07/18.
//

#include <Common/PluginApi/ApiException.h>
#include "PluginApiImpl.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
constexpr  int SendReplyTimeout = 2000;

std::unique_ptr<Common::PluginApi::IPluginApi> Common::PluginApi::IPluginApi::newPluginAPI( const std::string & pluginName, std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback)
{
    std::unique_ptr<Common::PluginApiImpl::PluginApiImpl> plugin(new Common::PluginApiImpl::PluginApiImpl(pluginName));
    plugin->setPluginCallback(pluginCallback);
    return  std::unique_ptr<Common::PluginApi::IPluginApi>(plugin.release());
}
////static std::unique_ptr<IPluginApi> newPluginAPI(const std::string& pluginName, std::shared_ptr<IPluginCallback> pluginCallback);


Common::PluginApiImpl::PluginApiImpl::PluginApiImpl(const std::string &pluginName)
: m_pluginName( pluginName), m_pluginCallbackHandler(), m_socket()
{
    m_context = Common::ZeroMQWrapper::createContext();
    auto requester = m_context->getRequester();
    requester->setTimeout(SendReplyTimeout);

    std::string mng_address = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();

    requester->connect(mng_address);

    m_socket = std::move(requester);

}

void Common::PluginApiImpl::PluginApiImpl::setPluginCallback(
        std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback)
{
    std::string pluginAddress = Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(m_pluginName);
    auto replier = m_context->getReplier();
    replier->setTimeout(SendReplyTimeout);
    replier->listen(pluginAddress);
    m_pluginCallbackHandler.reset(new PluginCallBackHandler(std::move(replier), pluginCallback) );
    m_pluginCallbackHandler->start();
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
