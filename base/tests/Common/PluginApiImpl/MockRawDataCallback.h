/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IRawDataCallback.h>
#include <Common/EventTypes/IEventType.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

class MockRawDataCallback: public Common::PluginApi::IRawDataCallback
{
public:
    MOCK_METHOD2(receiveData, void(const std::string& key, const std::string& data));
};
