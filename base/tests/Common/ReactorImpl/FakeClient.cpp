//
// Created by pair on 26/06/18.
//

#include "FakeClient.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"

FakeClient::~FakeClient()=default;

FakeClient::FakeClient(Common::ZeroMQWrapper::IContext &iContext, const std::string &address, int timeout)
{
    m_socketRequester = iContext.getRequester();
    if ( timeout != -1)
    {
        m_socketRequester->setTimeout(timeout);
    }

    m_socketRequester->connect(address);
}

std::vector<std::string> FakeClient::requestReply(const std::vector<std::string> &request)
{
    m_socketRequester->write(request);
    return m_socketRequester->read();
}
