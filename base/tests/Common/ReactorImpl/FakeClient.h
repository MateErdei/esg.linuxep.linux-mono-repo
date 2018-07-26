/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once



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



