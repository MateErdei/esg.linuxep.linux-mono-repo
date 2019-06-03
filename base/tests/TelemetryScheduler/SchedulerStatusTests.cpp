/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <TelemetryScheduler/TelemetrySchedulerImpl/SchedulerStatus.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/SchedulerStatusSerialiser.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <json.hpp>

using ::testing::StrictMock;
using namespace TelemetrySchedulerImpl;

class SchedulerStatusTests : public ::testing::Test
{
public:
    SchedulerStatus m_schedulerStatus;

    nlohmann::json m_jsonObject;
    nlohmann::json m_jsonObjectExtraKeys;
    nlohmann::json m_jsonObjectInvalidValue;

    const unsigned int m_scheduledTimeValue = 123456;
    const std::string m_invalidscheduledTimeValueType = "dfhk56gh";
    const std::string m_statusJsonString = R"({"scheduled-time":123456})";

    void SetUp() override
    {
        m_schedulerStatus.setTelemetryScheduledTime(m_scheduledTimeValue);
        m_jsonObject["scheduled-time"] = m_scheduledTimeValue;

        m_jsonObjectExtraKeys["scheduled-time"] = m_scheduledTimeValue;
        m_jsonObjectExtraKeys["SomeOtherunExpected"] = "Unexpected";

        m_jsonObjectInvalidValue["scheduled-time"] = m_invalidscheduledTimeValueType;
    }
};

TEST_F(SchedulerStatusTests, defaultConstrutor) // NOLINT
{
    SchedulerStatus schedulerStatus;

    EXPECT_EQ(0, schedulerStatus.getTelemetryScheduledTime());

    ASSERT_TRUE(schedulerStatus.isValid());
}

TEST_F(SchedulerStatusTests, serialise) // NOLINT
{
    std::string jsonString = TelemetrySchedulerImpl::SchedulerStatusSerialiser::serialise(m_schedulerStatus);
    EXPECT_EQ(m_statusJsonString, jsonString);
}

TEST_F(SchedulerStatusTests, deserialise) // NOLINT
{
    SchedulerStatus schedulerStatus =
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(m_statusJsonString);
    EXPECT_EQ(m_schedulerStatus, schedulerStatus);
}

TEST_F(SchedulerStatusTests, serialiseAndDeserialise) // NOLINT
{
    ASSERT_EQ(
        m_schedulerStatus,
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(
            TelemetrySchedulerImpl::SchedulerStatusSerialiser::serialise(m_schedulerStatus)));
}

TEST_F(SchedulerStatusTests, invalidJsonCannotBeDeserialised) // NOLINT
{
    std::string invalidJsonString = m_jsonObjectInvalidValue.dump();

    // Try to convert the invalidJsonString to a config object
    EXPECT_THROW(
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(invalidJsonString),
        std::runtime_error); // NOLINT
}

TEST_F(SchedulerStatusTests, brokenJsonCannotBeDeserialised) // NOLINT
{
    // Try to convert broken JSON to a config object
    EXPECT_THROW(
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise("imbroken:("), std::runtime_error); // NOLINT
}

TEST_F(SchedulerStatusTests, parseValidConfigJsonDirectlySucceeds) // NOLINT
{
    SchedulerStatus schedulerStatus =
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(m_statusJsonString);
    EXPECT_EQ(m_schedulerStatus, schedulerStatus);
}

TEST_F(SchedulerStatusTests, parseSupersetConfigJsonDirectlySucceeds) // NOLINT
{
    const std::string supersetTelemetryJson = R"(
        {
            "ExtraKey": "08546",
            "scheduled-time": 1237,
            "AnotherExtraKey": 199797
            })";
    SchedulerStatus schedulerStatus =
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(supersetTelemetryJson);
    EXPECT_EQ(1237UL, schedulerStatus.getTelemetryScheduledTime());
}

TEST_F(SchedulerStatusTests, parseInvalidValueThrows) // NOLINT
{
    const std::string supersetTelemetryJson = R"(
        {
            "scheduled-time": -1237
            })";
    EXPECT_THROW(
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(supersetTelemetryJson), std::runtime_error);
}

TEST_F(SchedulerStatusTests, serialiseInvalidValueThrows) // NOLINT
{
    SchedulerStatus schedulerStatus;
    schedulerStatus.setTelemetryScheduledTime(-12345);
    EXPECT_THROW(TelemetrySchedulerImpl::SchedulerStatusSerialiser::serialise(schedulerStatus), std::invalid_argument);
}

TEST_F(SchedulerStatusTests, SchedulerStatusEquality) // NOLINT
{
    SchedulerStatus a;
    SchedulerStatus b;

    a.setTelemetryScheduledTime(m_scheduledTimeValue);
    ASSERT_EQ(a, a);
    ASSERT_NE(a, b);

    b.setTelemetryScheduledTime(m_scheduledTimeValue);
    ASSERT_EQ(a, b);
}
