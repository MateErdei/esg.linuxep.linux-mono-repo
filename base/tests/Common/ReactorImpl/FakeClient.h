//
// Created by pair on 26/06/18.
//

#ifndef EVEREST_BASE_FAKECLIENT_H
#define EVEREST_BASE_FAKECLIENT_H


#include <Common/ZeroMQWrapper/IContext.h>
#include <vector>
#include <string>

class FakeClient
{
public:
    FakeClient(Common::ZeroMQWrapper::IContext& iContext, const std::string & address, int timeout );
    ~FakeClient();
    std::vector<std::string> requestReply( const std::vector<std::string> & request);
private:
    std::unique_ptr<Common::ZeroMQWrapper::ISocketRequester> m_socketRequester;

};


#endif //EVEREST_BASE_FAKECLIENT_H
