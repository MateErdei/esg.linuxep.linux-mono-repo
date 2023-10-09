/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "modules/Common/PluginProtocol/DataMessage.h"
#include "modules/Common/PluginProtocol/MessageBuilder.h"
#include "modules/Common/PluginProtocol/Protocol.h"
#include "modules/Common/ZMQWrapperApi/IContext.h"
#include "modules/Common/ZeroMQWrapper/IReadable.h"
#include "modules/Common/ZeroMQWrapper/ISocketReplier.h"

#include <cassert>

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
    void setReply(const Common::PluginProtocol::DataMessage& replyMessage)
    {
        Common::PluginProtocol::Protocol protocol;
        setReplyRaw(protocol.serialize(replyMessage));
    }
    void setReplyRaw(const Common::ZeroMQWrapper::IReadable::data_t& replyMessage) { m_replyMessage = replyMessage; }
    void doNotReply() { m_replyMessage.clear(); }

    void operator()(const Common::ZMQWrapperApi::IContextSharedPtr& context)
    {
        auto replier = context->getReplier();
        std::string address =
            Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        replier->listen(address);

        // handle registration
        Common::PluginProtocol::Protocol protocol;
        auto request = protocol.deserialize(replier->read());
        assert(request.m_command == Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER);
        Common::PluginProtocol::MessageBuilder messageBuilder("plugin");
        auto replyMessage = protocol.serialize(messageBuilder.replyAckMessage(request));
        replier->write(replyMessage);

        // handle single request

        // it does not care about the received request, it will always respond with the configured answer.
        replier->read();

        if (!m_replyMessage.empty())
        {
            replier->write(m_replyMessage);
        }
    }
};
