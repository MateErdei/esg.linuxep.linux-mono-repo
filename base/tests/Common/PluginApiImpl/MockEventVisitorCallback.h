/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IRawDataCallback.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

class MockEventVisitorCallback: public Common::PluginApi::IRawDataCallback
{
public:
    MOCK_METHOD2(receiveData, void(const std::string& key, const std::string& data));
};
