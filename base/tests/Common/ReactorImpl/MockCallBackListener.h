/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once



#include "Common/Reactor/ICallbackListener.h"
#include <gmock/gmock.h>
using namespace ::testing;
using namespace Common::Reactor;


class MockCallBackListener: public ICallbackListener
{
public:
    MOCK_METHOD1( messageHandler, void (Common::ZeroMQWrapper::IReadable::data_t));
};


