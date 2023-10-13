/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once

#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketSubscriber.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/SubscriberLib/ISubscriber.h>

using namespace ::testing;
using namespace Common;

class MockSubscriberLib : public SubscriberLib::ISubscriber
{
public:
    MOCK_METHOD0(stop, void(void));
    MOCK_METHOD0(start, void(void));
    MOCK_METHOD0(restart, void(void));
    MOCK_METHOD0(getRunningStatus, bool(void));
};
