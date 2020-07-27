/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <modules/CommsComponent/OtherSideApi.h>
#include <modules/CommsComponent/AsyncMessager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockOtherSideApi : public CommsComponent::IOtherSideApi
{
public:
    MOCK_METHOD1(pushMessage, void(const std::string &));
    MOCK_METHOD0(notifyOtherSideAndClose, void());
    MOCK_METHOD1(setNotifyOnClosure, void(NotifySocketClosed));
};

