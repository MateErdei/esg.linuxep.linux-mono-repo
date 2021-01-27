// Copyright 2020 Sophos Limited.

#include <UpdateSchedulerImpl/stateMachines/EventStateMachine.h>

#include <UpdateSchedulerImpl/stateMachines/SEUErrors.h>
#include "TimeStamp.h"

//#include <CppCommon/GTestIncluder.h>
#include <gtest/gtest.h>

using namespace StateLib;
using namespace std::chrono_literals;

namespace
{
    class EventStateMachineTests : public ::testing::Test
    {
    };

//    std::ostream& operator<<(std::ostream& os, const StateData::EventMachineState& state)
//    {
//        os
//            << "{lastError=" << state.lastError
//            << ",lastTime=" << Utilities::MessageTimeStamp(state.lastTime)
//            << "}";
//        return os;
//    }
} // namespace

TEST_F(EventStateMachineTests, NoFailuresThrottlesSuccessEvents)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState dms{};
    DownloadStateMachine dsm{ dms, mockTime };

    StateData::InstallMachineState ims{};
    InstallStateMachine ism{ ims, mockTime };

    StateData::EventMachineState mockData{};

    EventStateMachine sm{ dsm, ism, mockData };

    for (auto i = 0; i < 100; ++i)
    {
        mockTime += StateData::EventMachineState::throttlePeriod / 2;
        EXPECT_EQ(0 != i % 2, sm.Discard(0, mockTime));
    }
}

TEST_F(EventStateMachineTests, ThrottlesTemporaryNetworkErrors)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState dms{};
    DownloadStateMachine dsm{ dms, mockTime };

    StateData::InstallMachineState ims{};
    InstallStateMachine ism{ ims, mockTime };

    StateData::EventMachineState mockData{};

    EventStateMachine sm{ dsm, ism, mockData };

    for (auto i = 0; i < 100; ++i)
    {
        mockTime += StateData::EventMachineState::throttlePeriod;
        EXPECT_TRUE(sm.Discard(IDS_RMS_CONNECTIONFAILED, mockTime));
    }
}

TEST_F(EventStateMachineTests, DoesNotThrottlePermanentNetworkErrors)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState dms{};
    dms.credit = 0;
    DownloadStateMachine dsm{ dms, mockTime };

    StateData::InstallMachineState ims{};
    InstallStateMachine ism{ ims, mockTime };

    StateData::EventMachineState mockData{};

    EventStateMachine sm{ dsm, ism, mockData };

    for (auto i = 0; i < 100; ++i)
    {
        mockTime += StateData::EventMachineState::throttlePeriod;
        EXPECT_FALSE(sm.Discard(IDS_RMS_CONNECTIONFAILED, mockTime));
    }
}

TEST_F(EventStateMachineTests, ThrottlesTemporaryInstallErrors)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState dms{};
    DownloadStateMachine dsm{ dms, mockTime };

    StateData::InstallMachineState ims{};
    InstallStateMachine ism{ ims, mockTime };

    StateData::EventMachineState mockData{};

    EventStateMachine sm{ dsm, ism, mockData };

    for (auto i = 0; i < 100; ++i)
    {
        mockTime += StateData::EventMachineState::throttlePeriod;
        EXPECT_TRUE(sm.Discard(IDS_RMS_INST_ERROR, mockTime));
    }
}

TEST_F(EventStateMachineTests, DoesNotThrottlePermanentInstallErrors)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState dms{};
    DownloadStateMachine dsm{ dms, mockTime };

    StateData::InstallMachineState ims{};
    ims.credit = 0;
    InstallStateMachine ism{ ims, mockTime };

    StateData::EventMachineState mockData{};

    EventStateMachine sm{ dsm, ism, mockData };

    for (auto i = 0; i < 100; ++i)
    {
        mockTime += StateData::EventMachineState::throttlePeriod;
        EXPECT_FALSE(sm.Discard(IDS_RMS_INST_ERROR, mockTime));
    }
}

TEST_F(EventStateMachineTests, Reset)
{
    auto mockTime{ Utilities::MakeTimePoint(2000, 1, 1, 0, 0, 0, 0) };

    StateData::DownloadMachineState dms{};
    DownloadStateMachine dsm{ dms, mockTime };

    StateData::InstallMachineState ims{};
    InstallStateMachine ism{ ims, mockTime };

    StateData::EventMachineState mockData{};

    EventStateMachine sm{ dsm, ism, mockData };
    EXPECT_FALSE(sm.Discard(0, mockTime));
    EXPECT_TRUE(sm.Discard(0, mockTime));
    EXPECT_TRUE(sm.Discard(0, mockTime));

    mockData.lastTime = {};
    sm.Reset(mockData);
    EXPECT_FALSE(sm.Discard(0, mockTime));
    EXPECT_TRUE(sm.Discard(0, mockTime));
    EXPECT_TRUE(sm.Discard(0, mockTime));
}