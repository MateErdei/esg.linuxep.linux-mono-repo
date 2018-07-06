/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_SINGLERESPONSESERVER_H
#define EVEREST_BASE_SINGLERESPONSESERVER_H
#include "Common/ZeroMQWrapper/IReadable.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/PluginProtocol/DataMessage.h"
#include "Common/PluginProtocol/Protocol.h"
#include "Common/PluginApiImpl/SharedSocketContext.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include <cassert>
#include <Common/PluginProtocol/MessageBuilder.h>

/** This class is to be used for tests to facilitate the interaction with client/server of the api, allowing for
 *  single read/write operation.
 *  It's main method is the operator (), which allows it to be given to a thread and work as a functor.
 */
class SingleResponseServer
{

    Common::ZeroMQWrapper::IReadable::data_t m_replyMessage;
public:
    SingleResponseServer() = default;
    ~SingleResponseServer() = default;
    void setReply( const Common::PluginProtocol::DataMessage & replyMessage)
    {
        Common::PluginProtocol::Protocol protocol;
        setReplyRaw(protocol.serialize(replyMessage));
    }
    void setReplyRaw( Common::ZeroMQWrapper::IReadable::data_t replyMessage)
    {
        m_replyMessage = replyMessage;
    }
    void doNotReply()
    {
        m_replyMessage.clear();
    }

    void operator()(Common::ZeroMQWrapper::IContext & context)
    {
        auto replier = context.getReplier();
        std::string address = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        replier->listen(address );

        // handle registration
        Common::PluginProtocol::Protocol protocol;
        auto request = protocol.deserialize(replier->read());
        assert( request.Command == Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER);
        Common::PluginProtocol::MessageBuilder messageBuilder( "plugin", "v1");
        auto replyMessage = protocol.serialize( messageBuilder.replyAckMessage(request) );
        replier->write(replyMessage);


        //handle single request

        // it does not care about the received request, it will always respond with the configured answer.
        replier->read();

        if ( !m_replyMessage.empty())
        {
            replier->write(m_replyMessage);
        }
    }
};
#endif //EVEREST_BASE_SINGLERESPONSESERVER_H
