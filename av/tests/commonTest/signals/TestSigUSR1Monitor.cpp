// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "modules/common/signals/SigUSR1Monitor.h"
#include "tests/common/LogInitializedTests.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    class TestSigUSR1Monitor: public LogOffInitializedTests
    {
    };

    class MockReloadable : public sspl::sophosthreatdetectorimpl::IReloadable
    {
    public:
        MOCK_METHOD(void, update, (), (override));
        MOCK_METHOD(void, reload, (), (override));
    };
}

using sspl::sophosthreatdetectorimpl::SigUSR1Monitor;



TEST_F(TestSigUSR1Monitor, testConstruction)
{
    auto reloadable = std::make_shared<MockReloadable>();
    SigUSR1Monitor monitor(reloadable);
}

TEST_F(TestSigUSR1Monitor, triggerNotified)
{
    auto reloadable = std::make_shared<MockReloadable>();
    SigUSR1Monitor monitor(reloadable);
    ::kill(::getpid(), SIGUSR1);
    monitor.triggered();

}
