/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/IReadable.h"

#include <string>
#include <vector>

class FakeClient
{
public:
    FakeClient(Common::ZMQWrapperApi::IContext& iContext, const std::string& address, int timeout);
    ~FakeClient();
    Common::ZeroMQWrapper::IReadable::data_t requestReply(const Common::ZeroMQWrapper::IReadable::data_t& request);

private:
    std::unique_ptr<Common::ZeroMQWrapper::ISocketRequester> m_socketRequester;
};
