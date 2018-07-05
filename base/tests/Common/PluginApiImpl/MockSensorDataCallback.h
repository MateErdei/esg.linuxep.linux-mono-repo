/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_MOCKSENSORDATACALLBACK_H
#define EVEREST_BASE_MOCKSENSORDATACALLBACK_H

#include <Common/PluginApi/ISensorDataCallback.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"


class MockSensorDataCallback: public Common::PluginApi::ISensorDataCallback
{
public:
    MOCK_METHOD2(receiveData, void(const std::string &, const std::string &));
};

#endif //EVEREST_BASE_MOCKSENSORDATACALLBACK_H
