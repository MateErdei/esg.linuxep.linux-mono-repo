/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeClient.h"

#include "Common/ZeroMQWrapper/ISocketRequester.h"

FakeClient::~FakeClient() = default;

FakeClient::FakeClient(Common::ZMQWrapperApi::IContext& iContext, const std::string& address, int timeout)
{
    m_socketRequester = iContext.getRequester();
    if (timeout != -1)
    {
        m_socketRequester->setConnectionTimeout(timeout);
    }

    m_socketRequester->connect(address);
}

Common::ZeroMQWrapper::IReadable::data_t FakeClient::requestReply(
    const Common::ZeroMQWrapper::IReadable::data_t& request)
{
    m_socketRequester->write(request);
    return m_socketRequester->read();
}
