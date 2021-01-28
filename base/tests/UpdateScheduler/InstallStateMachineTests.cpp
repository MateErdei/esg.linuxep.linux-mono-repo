/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateSchedulerImpl/stateMachinesModule/InstallStateMachine.h>

#include "TimeStamp.h"

#include <gtest/gtest.h>

using namespace stateMachinesModule;
using namespace std::chrono_literals;

namespace
{
    class InstallStateMachineTests : public ::testing::Test
    {
    };

} // namespace

TEST_F(InstallStateMachineTests, NoFailures)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};
    mockData.lastGood = mockTime - 1h;

    InstallStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    for (auto i = 0; i < 10; ++i)
    {
        mockTime += 1h;
        sm.SignalInstallResult(StateData::StatusEnum::good, mockTime);
        mockData.lastGood = mockTime;
        EXPECT_EQ(mockData, sm.CurrentState());
    }
}

TEST_F(InstallStateMachineTests, TemporaryFailureWithRecovery)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};
    mockData.failedSince = mockTime + 1h;

    InstallStateMachine sm{ mockData, mockTime };
    mockData.lastGood = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());

    static_assert(StateData::InstallMachineState::defaultCredit > 2, "Test assumption");
    for (auto i = 0; i < 2; ++i)
    {
        mockTime += 1h;
        sm.SignalInstallResult(StateData::StatusEnum::bad, mockTime);
        --mockData.credit;
        EXPECT_EQ(mockData, sm.CurrentState());
    }

    mockTime += 1h;
    sm.SignalInstallResult(StateData::StatusEnum::good, mockTime);
    mockData.credit = StateData::InstallMachineState::defaultCredit;
    mockData.lastGood = mockTime;
    mockData.failedSince = {};
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(InstallStateMachineTests, PersistentFailure)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};

    InstallStateMachine sm{ mockData, mockTime };
    mockData.lastGood = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());

    static_assert(StateData::InstallMachineState::defaultCredit < 10, "Test assumption");
    static_assert(StateData::InstallMachineState::defaultCredit > 0, "Test assumption");
    for (auto i = 0; i < 10; ++i)
    {
        sm.SignalInstallResult(StateData::StatusEnum::bad, mockTime + std::chrono::hours(i));
    }

    mockData.credit = 0;
    mockData.failedSince = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(InstallStateMachineTests, RecoveryFromPersistentFailure)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};
    mockData.credit = 0;
    mockData.failedSince = mockTime - 1h;

    InstallStateMachine sm{ mockData, mockTime };
    EXPECT_EQ(mockData, sm.CurrentState());

    mockTime += 1h;
    sm.SignalInstallResult(StateData::StatusEnum::good, mockTime);
    mockData.credit = StateData::InstallMachineState::defaultCredit;
    mockData.lastGood = mockTime;
    mockData.failedSince = {};
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(InstallStateMachineTests, InitialisationOnSuccess)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};
    InstallStateMachine sm{ mockData, mockTime };

    mockData.lastGood = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(InstallStateMachineTests, InitialisationOnTemporaryFailure)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};
    mockData.credit = 1;
    InstallStateMachine sm{ mockData, mockTime };

    mockData.failedSince = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(InstallStateMachineTests, InitialisationOnPermanentFailure)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};
    mockData.credit = 0;
    InstallStateMachine sm{ mockData, mockTime };

    mockData.failedSince = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());
}


TEST_F(InstallStateMachineTests, TemporaryFailureWithLastGood)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};
    mockData.credit = 1;
    mockData.lastGood = mockTime - 1h;
    InstallStateMachine sm{ mockData, mockTime };

    mockData.failedSince = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());
}

TEST_F(InstallStateMachineTests, PermanentFailureWithLastGood)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::InstallMachineState mockData{};
    mockData.credit = 0;
    mockData.lastGood = mockTime - 1h;
    InstallStateMachine sm{ mockData, mockTime };

    mockData.failedSince = mockTime;
    EXPECT_EQ(mockData, sm.CurrentState());
}