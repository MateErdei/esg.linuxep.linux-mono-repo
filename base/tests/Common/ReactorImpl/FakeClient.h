//
// Created by pair on 26/06/18.
//

#ifndef EVEREST_BASE_FAKECLIENT_H
#define EVEREST_BASE_FAKECLIENT_H


#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapper/IReadable.h"
#include <vector>
#include <string>

class FakeClient
{

public:
    FakeClient(Common::ZeroMQWrapper::IContext& iContext, const std::string & address, int timeout );
    ~FakeClient();
    Common::ZeroMQWrapper::IReadable::data_t requestReply( const Common::ZeroMQWrapper::IReadable::data_t & request);
private:
    std::unique_ptr<Common::ZeroMQWrapper::ISocketRequester> m_socketRequester;

};


#endif //EVEREST_BASE_FAKECLIENT_H
