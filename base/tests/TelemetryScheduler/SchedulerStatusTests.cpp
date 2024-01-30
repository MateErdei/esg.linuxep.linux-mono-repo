// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "TelemetryScheduler/TelemetrySchedulerImpl/SchedulerStatus.h"
#include "TelemetryScheduler/TelemetrySchedulerImpl/SchedulerStatusSerialiser.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

using ::testing::StrictMock;
using namespace TelemetrySchedulerImpl;
using namespace std::chrono;

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

    static time_t toEpoch(const SchedulerStatus::time_point& t)
    {
        return SchedulerStatus::clock::to_time_t(t);
    }

    static SchedulerStatus::time_point fromEpoch(const time_t& t)
    {
        return SchedulerStatus::clock::from_time_t(t);
    }

    void SetUp() override
    {
        m_schedulerStatus.setTelemetryScheduledTime(fromEpoch(m_scheduledTimeValue));
        m_jsonObject["scheduled-time"] = m_scheduledTimeValue;

        m_jsonObjectExtraKeys["scheduled-time"] = m_scheduledTimeValue;
        m_jsonObjectExtraKeys["SomeOtherunExpected"] = "Unexpected";

        m_jsonObjectInvalidValue["scheduled-time"] = m_invalidscheduledTimeValueType;
    }

};

TEST_F(SchedulerStatusTests, defaultConstrutor)
{
    SchedulerStatus schedulerStatus;

    EXPECT_EQ(0, toEpoch(schedulerStatus.getTelemetryScheduledTime()));

    auto lastTime = schedulerStatus.getLastTelemetryStartTime();
    EXPECT_FALSE(lastTime);
    EXPECT_EQ(lastTime, std::nullopt);

    ASSERT_FALSE(schedulerStatus.isValid());

}

TEST_F(SchedulerStatusTests, serialise)
{
    std::string jsonString = TelemetrySchedulerImpl::SchedulerStatusSerialiser::serialise(m_schedulerStatus);
    EXPECT_EQ(m_statusJsonString, jsonString);
}

TEST_F(SchedulerStatusTests, deserialise)
{
    SchedulerStatus schedulerStatus =
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(m_statusJsonString);
    EXPECT_EQ(m_schedulerStatus, schedulerStatus);
}

TEST_F(SchedulerStatusTests, serialiseAndDeserialise)
{
    ASSERT_EQ(
        m_schedulerStatus,
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(
            TelemetrySchedulerImpl::SchedulerStatusSerialiser::serialise(m_schedulerStatus)));
}

TEST_F(SchedulerStatusTests, invalidJsonCannotBeDeserialised)
{
    std::string invalidJsonString = m_jsonObjectInvalidValue.dump();

    // Try to convert the invalidJsonString to a config object
    EXPECT_THROW(
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(invalidJsonString),
        std::runtime_error);
}

TEST_F(SchedulerStatusTests, brokenJsonCannotBeDeserialised)
{
    // Try to convert broken JSON to a config object
    EXPECT_THROW(
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise("imbroken:("), std::runtime_error);
}

TEST_F(SchedulerStatusTests, parseValidConfigJsonDirectlySucceeds)
{
    SchedulerStatus schedulerStatus =
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(m_statusJsonString);
    EXPECT_EQ(m_schedulerStatus, schedulerStatus);
}

TEST_F(SchedulerStatusTests, parseSupersetConfigJsonDirectlySucceeds)
{
    const std::string supersetTelemetryJson = R"(
        {
            "ExtraKey": "08546",
            "scheduled-time": 1237,
            "AnotherExtraKey": 199797
            })";
    SchedulerStatus schedulerStatus = SchedulerStatusSerialiser::deserialise(supersetTelemetryJson);
    EXPECT_EQ(1237UL, toEpoch(schedulerStatus.getTelemetryScheduledTime()));
}

TEST_F(SchedulerStatusTests, parseInvalidValueThrows)
{
    const std::string supersetTelemetryJson = R"(
        {
            "scheduled-time": -1237
            })";
    EXPECT_THROW(
        TelemetrySchedulerImpl::SchedulerStatusSerialiser::deserialise(supersetTelemetryJson), std::runtime_error);
}

TEST_F(SchedulerStatusTests, serialiseInvalidValueThrows)
{
    SchedulerStatus schedulerStatus;
    schedulerStatus.setTelemetryScheduledTime(fromEpoch(-12345));
    EXPECT_THROW(SchedulerStatusSerialiser::serialise(schedulerStatus), std::invalid_argument);
}

TEST_F(SchedulerStatusTests, SchedulerStatusEquality)
{
    SchedulerStatus a;
    SchedulerStatus b;

    a.setTelemetryScheduledTime(fromEpoch(m_scheduledTimeValue));
    ASSERT_EQ(a, a);
    ASSERT_NE(a, b);

    b.setTelemetryScheduledTime(fromEpoch(m_scheduledTimeValue));
    ASSERT_EQ(a, b);
}

TEST_F(SchedulerStatusTests, systemClockValueTranslatedCorrectly)
{
    SchedulerStatus statusIn;
    auto timeIn = system_clock::now();
    statusIn.setTelemetryScheduledTime(timeIn);
    std::string json = SchedulerStatusSerialiser::serialise(statusIn);
    SchedulerStatus statusOut = SchedulerStatusSerialiser::deserialise(json);
    auto timeOut = statusOut.getTelemetryScheduledTime();

    EXPECT_EQ(
        duration_cast<seconds>(timeIn.time_since_epoch()).count(),
        duration_cast<seconds>(timeOut.time_since_epoch()).count());
}

TEST_F(SchedulerStatusTests, epochTranslatedCorrectly)
{
    SchedulerStatus statusIn;
    statusIn.setTelemetryScheduledTime(fromEpoch(0));
    std::string json = SchedulerStatusSerialiser::serialise(statusIn);
    SchedulerStatus statusOut = SchedulerStatusSerialiser::deserialise(json);
    auto timeOut = statusOut.getTelemetryScheduledTime();

    system_clock::time_point epoch;

    EXPECT_EQ(
        duration_cast<seconds>(epoch.time_since_epoch()).count(),
        duration_cast<seconds>(timeOut.time_since_epoch()).count());
}

TEST_F(SchedulerStatusTests, storeLastTime)
{
    SchedulerStatus status;
    auto now = SchedulerStatus::clock::now();
    status.setLastTelemetryStartTime(now);
    EXPECT_EQ(status.getLastTelemetryStartTime().value(), now);
}

TEST_F(SchedulerStatusTests, serialiseLastTime)
{
    SchedulerStatus status;
    auto now = SchedulerStatus::clock::now();
    status.setTelemetryScheduledTime(now); // scheduled time required for serialisation
    status.setLastTelemetryStartTime(now);

    auto serialised = SchedulerStatusSerialiser::serialise(status);

    std::cerr << serialised << '\n';

    auto unserialised = SchedulerStatusSerialiser::deserialise(serialised);

    auto lastTime = unserialised.getLastTelemetryStartTime();
    ASSERT_TRUE(lastTime);

    auto nowSeconds = toEpoch(now);
    auto actualSeconds = toEpoch(lastTime.value());
    EXPECT_EQ(actualSeconds, nowSeconds);
}