//
// Created by pair on 25/06/18.
//

#ifndef EVEREST_BASE_MOCKSHUTDOWNLISTENER_H
#define EVEREST_BASE_MOCKSHUTDOWNLISTENER_H

#include "IShutdownListener.h"

#include <gmock/gmock.h>
using namespace ::testing;
using namespace Common::Reactor;


class MockShutdownListener: public IShutdownListener
{
public:
    MOCK_METHOD0( notifyShutdownRequested, void());
};

#endif //EVEREST_BASE_MOCKSHUTDOWNLISTENER_H
