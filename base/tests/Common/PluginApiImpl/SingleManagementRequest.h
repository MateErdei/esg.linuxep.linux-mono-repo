/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/PluginProtocol/DataMessage.h"
#include "modules/Common/PluginProtocol/Protocol.h"
#include "modules/Common/ZMQWrapperApi/IContext.h"
#include "modules/Common/ZeroMQWrapper/IReadable.h"
#include "modules/Common/ZeroMQWrapper/ISocketRequester.h"

#include "modules/Common/ApplicationConfiguration/IApplicationPathManager.h"

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
