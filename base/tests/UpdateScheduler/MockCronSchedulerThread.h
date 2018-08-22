/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <gmock/gmock.h>
#include <UpdateSchedulerImpl/ICronSchedulerThread.h>

using namespace ::testing;


class MockCronSchedulerThread
        : public UpdateSchedulerImpl::ICronSchedulerThread
{
public:
    MOCK_METHOD0(start, void());

    MOCK_METHOD0(requestStop, void());

    MOCK_METHOD0(reset, void());

    MOCK_METHOD1(setPeriodTime, void(UpdateSchedulerImpl::ICronSchedulerThread::DurationTime));
};
