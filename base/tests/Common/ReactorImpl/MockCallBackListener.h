//
// Created by pair on 25/06/18.
//

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


