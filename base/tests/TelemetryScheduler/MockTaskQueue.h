// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "TelemetryScheduler/TelemetrySchedulerImpl/ITaskQueue.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockTaskQueue : public TelemetrySchedulerImpl::ITaskQueue
{
public:
    MOCK_METHOD(void, push, (TelemetrySchedulerImpl::SchedulerTask), (override));
    MOCK_METHOD(void, pushPriority, (TelemetrySchedulerImpl::SchedulerTask), (override));
    MOCK_METHOD(TelemetrySchedulerImpl::SchedulerTask, pop, (), (override));
    MOCK_METHOD(bool, pop, (TelemetrySchedulerImpl::SchedulerTask & task, int timeout), (override));
};