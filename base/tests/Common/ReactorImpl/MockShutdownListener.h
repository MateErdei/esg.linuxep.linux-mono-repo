//
// Created by pair on 25/06/18.
//

#pragma once


#include "IShutdownListener.h"

#include <gmock/gmock.h>
using namespace ::testing;
using namespace Common::Reactor;


class MockShutdownListener: public IShutdownListener
{
public:
    MOCK_METHOD0( notifyShutdownRequested, void());
};


