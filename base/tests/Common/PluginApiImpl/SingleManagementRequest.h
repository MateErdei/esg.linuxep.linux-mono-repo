/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_SINGLEMANAGEMENTREQUEST_H
#define EVEREST_BASE_SINGLEMANAGEMENTREQUEST_H


#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include "Common/ZeroMQWrapper/IReadable.h"
#include "Common/PluginApi/DataMessage.h"
#include "Common/PluginApiImpl/Protocol.h"
#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"

class SingleManagementRequest
{
public:
    SingleManagementRequest() = default;
    ~SingleManagementRequest() = default;
    Common::PluginApi::DataMessage triggerRequest(Common::ZeroMQWrapper::IContext & context, const Common::PluginApi::DataMessage & requestMessage)
    {
        Common::PluginApiImpl::Protocol protocol;
        auto rawMessage = protocol.serialize(requestMessage);
        auto rawReply = triggerRawRequest(context, rawMessage);
        return protocol.deserialize(rawReply);
    }
    Common::ZeroMQWrapper::IReadable::data_t triggerRawRequest(Common::ZeroMQWrapper::IContext & context, Common::ZeroMQWrapper::IReadable::data_t message)
    {
        auto requester = context.getRequester();
        std::string address = Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress("plugin");
        requester->connect(address);

        requester->write(message);
        return requester->read();
    }

};
#endif //EVEREST_BASE_SINGLEMANAGEMENTREQUEST_H
