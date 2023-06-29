/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <UpdateScheduler/ICronSchedulerThread.h>
#include <gmock/gmock.h>

#include <string>

using namespace ::testing;

class MockCronSchedulerThread : public UpdateScheduler::ICronSchedulerThread
{
public:
    MOCK_METHOD(void, start, ());

    MOCK_METHOD(void, requestStop, ());

    MOCK_METHOD(void, reset, ());

    MOCK_METHOD(void, setPeriodTime, (UpdateScheduler::ICronSchedulerThread::DurationTime));

    MOCK_METHOD(void, setUpdateOnStartUp, (bool));
};