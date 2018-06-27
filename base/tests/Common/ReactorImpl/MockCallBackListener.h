//
// Created by pair on 25/06/18.
//

#ifndef EVEREST_BASE_MOCKCALLBACKLISTENER_H
#define EVEREST_BASE_MOCKCALLBACKLISTENER_H


#include "Common/Reactor/ICallbackListener.h"
#include <gmock/gmock.h>
using namespace ::testing;
using namespace Common::Reactor;


class MockCallBackListener: public ICallbackListener
{
public:
    MOCK_METHOD1( process, void (std::vector<std::string>));
};

#endif //EVEREST_BASE_MOCKCALLBACKLISTENER_H
