/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/PluginProtocol/DataMessage.h"
#include "Common/PluginProtocol/Protocol.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/IReadable.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

class SingleManagementRequest
{
public:
    SingleManagementRequest() = default;
    ~SingleManagementRequest() = default;
    Common::PluginProtocol::DataMessage triggerRequest(
        const Common::ZMQWrapperApi::IContextSharedPtr& context,
        const Common::PluginProtocol::DataMessage& requestMessage)
    {
        Common::PluginProtocol::Protocol protocol;
        auto rawMessage = protocol.serialize(requestMessage);
        auto rawReply = triggerRawRequest(context, rawMessage);
        return protocol.deserialize(rawReply);
    }
    Common::ZeroMQWrapper::IReadable::data_t triggerRawRequest(
        const Common::ZMQWrapperApi::IContextSharedPtr& context,
        const Common::ZeroMQWrapper::IReadable::data_t& message)
    {
        auto requester = context->getRequester();
        std::string address =
            Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress("plugin");
        requester->connect(address);

        requester->write(message);
        return requester->read();
    }
};
