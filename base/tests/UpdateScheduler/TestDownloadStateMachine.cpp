/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TimeStamp.h"

#include <UpdateSchedulerImpl/stateMachinesModule/DownloadStateMachine.h>

#include <ostream>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace stateMachinesModule;
using namespace std::chrono_literals;

namespace
{
    class DownloadStateMachineTests : public ::testing::Test
    {
    };

} // namespace

TEST_F(DownloadStateMachineTests, MachineStateDefaultsToSuccess)
{
    StateData::DownloadMachineState mockData{};
    EXPECT_GT(mockData.credit, 0U);
}

TEST_F(DownloadStateMachineTests, NoFailures)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};

    DownloadStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    for(auto i = 0 ; i < StateData::DownloadMachineState::defaultCredit + 3 ; ++i)
    {
        mockTime += 1h;
        sm.SignalDownloadResult(StateData::DownloadResultEnum::good, mockTime);
        EXPECT_EQ(mockData, sm.CurrentState());
    }
}

TEST_F(DownloadStateMachineTests, TemporaryFailureWithRecovery)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};
    mockData.failedSince = mockTime + 1h;

    DownloadStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    static_assert(StateData::DownloadMachineState::defaultStandardCredit >= 4, "Test assumption");
    for (auto i = 0; i < StateData::DownloadMachineState::defaultStandardCredit - 1; ++i)
    {
        mockTime += 1h;
        sm.SignalDownloadResult(StateData::DownloadResultEnum::bad, mockTime);
        mockData.credit -= StateData::DownloadMachineState::noNetworkCreditMultiplicationFactor;
        EXPECT_EQ(mockData, sm.CurrentState());
    }

    mockTime += 1h;
    sm.SignalDownloadResult(StateData::DownloadResultEnum::good,  mockTime);
    mockData.credit = StateData::DownloadMachineState::defaultCredit;
    mockData.failedSince = {};
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(DownloadStateMachineTests, TemporaryNetworkLossWithRecovery)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};
    mockData.failedSince = mockTime + 1h;

    DownloadStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    static_assert(StateData::DownloadMachineState::defaultCredit >= 72, "Test assumption");
    for (auto i = 0; i < StateData::DownloadMachineState::defaultCredit - 1; ++i)
    {
        mockTime += 1h;
        sm.SignalDownloadResult(StateData::DownloadResultEnum::noNetwork, mockTime);
        --mockData.credit;
        EXPECT_EQ(mockData, sm.CurrentState());
    }

    mockTime += 1h;
    sm.SignalDownloadResult(StateData::DownloadResultEnum::good, mockTime);
    mockData.credit = StateData::DownloadMachineState::defaultCredit;
    mockData.failedSince = {};
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(DownloadStateMachineTests, TemporaryFailureWithIntermittentNetworkLossWithRecovery)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};
    mockData.failedSince = mockTime + 1h;

    DownloadStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    static_assert(StateData::DownloadMachineState::defaultStandardCredit >= 4, "Test assumption");
    for (auto i = 0; i < StateData::DownloadMachineState::defaultStandardCredit / 2 - 1; ++i)
    {
        mockTime += 1h;
        sm.SignalDownloadResult(StateData::DownloadResultEnum::bad, mockTime);
        mockData.credit -= StateData::DownloadMachineState::noNetworkCreditMultiplicationFactor;
        EXPECT_EQ(mockData, sm.CurrentState());
    }

    static_assert(StateData::DownloadMachineState::defaultCredit >= 72, "Test assumption");
    for (auto i = 0; i < StateData::DownloadMachineState::defaultCredit / 2 - 1; ++i)
    {
        mockTime += 1h;
        sm.SignalDownloadResult(StateData::DownloadResultEnum::noNetwork, mockTime);
        --mockData.credit;
        EXPECT_EQ(mockData, sm.CurrentState());
    }

    mockTime += 1h;
    sm.SignalDownloadResult(StateData::DownloadResultEnum::good, mockTime);
    mockData.credit = StateData::DownloadMachineState::defaultCredit;
    mockData.failedSince = {};
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(DownloadStateMachineTests, PersistentFailure)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};

    DownloadStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    mockData.failedSince = mockTime + 1h;

    for (auto i = 0; i < StateData::DownloadMachineState::defaultStandardCredit; ++i)
    {
        mockTime += 1h;
        sm.SignalDownloadResult(StateData::DownloadResultEnum::bad, mockTime);
    }
    mockData.credit = 0;
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(DownloadStateMachineTests, PersistentNetworkLoss)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};

    DownloadStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    mockData.failedSince = mockTime + 1h;

    for (auto i = 0; i < StateData::DownloadMachineState::defaultCredit; ++i)
    {
        mockTime += 1h;
        sm.SignalDownloadResult(StateData::DownloadResultEnum::noNetwork, mockTime);
    }
    mockData.credit = 0;
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(DownloadStateMachineTests, RecoveryFromPersistentFailure)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};
    mockData.credit = 0;
    mockData.failedSince = mockTime;

    DownloadStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    mockTime += 1h;
    sm.SignalDownloadResult(StateData::DownloadResultEnum::good, mockTime);
    static_assert(StateData::DownloadMachineState::defaultCredit > 1, "Test assumption");
    mockData.credit = StateData::DownloadMachineState::defaultCredit;
    mockData.failedSince = {};
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(DownloadStateMachineTests, InitialisationOnSuccess)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};
    DownloadStateMachine sm{ mockData, mockTime };

    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(DownloadStateMachineTests, InitialisationOnTemporaryFailure)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};
    mockData.credit = 1;
    DownloadStateMachine sm{ mockData, mockTime };

    mockData.failedSince = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(DownloadStateMachineTests, InitialisationOnPermanentFailure)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState mockData{};
    mockData.credit = 0;
    DownloadStateMachine sm{ mockData, mockTime };

    mockData.failedSince = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());
}