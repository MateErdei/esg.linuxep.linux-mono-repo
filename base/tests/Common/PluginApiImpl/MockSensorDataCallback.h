/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


#include <Common/PluginApi/ISensorDataCallback.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"


class MockSensorDataCallback: public Common::PluginApi::ISensorDataCallback
{
public:
    MOCK_METHOD2(receiveData, void(const std::string &, const std::string &));
};


