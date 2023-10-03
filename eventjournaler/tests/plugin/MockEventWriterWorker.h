// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "EventWriterWorkerLib/IEventWriterWorker.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace Common;

class MockEventWriterWorker : public EventWriterLib::IEventWriterWorker
{
public:
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, beginStop, (), (override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, restart, (), (override));
    MOCK_METHOD(bool, getRunningStatus, (), (override));
    MOCK_METHOD(void, checkAndPruneTruncatedEvents, (const std::string&), (override));
};
