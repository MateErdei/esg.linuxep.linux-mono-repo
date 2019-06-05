/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <TelemetryScheduler/TelemetrySchedulerImpl/ITaskQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockTaskQueue : public TelemetrySchedulerImpl::ITaskQueue
{
public:
    MOCK_METHOD1(push, void(TelemetrySchedulerImpl::Task));
    MOCK_METHOD1(pushPriority, void(TelemetrySchedulerImpl::Task));
    MOCK_METHOD0(pop, TelemetrySchedulerImpl::Task());
};