// Copyright 2022, Sophos Limited.  All rights reserved.

# define TEST_PUBLIC public

#include "OnAccessImplMemoryAppenderUsingTests.h"

#include "sophos_on_access_process/onaccessimpl/OnAccessTelemetryUtility.h"

#include <gtest/gtest.h>

using namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry;
using namespace testing;


class TestOnAccessTelemetryUtility : public OnAccessImplMemoryAppenderUsingTests
{
protected:
    OnAccessTelemetryUtility m_TelemetryUtility;

    void initialiseTelemetryWithValues(unsigned int dropped, unsigned long events, unsigned int errors, unsigned long scans)
    {
        m_TelemetryUtility.m_eventsReceived.store(events);
        m_TelemetryUtility.m_eventsDropped.store(dropped);
        m_TelemetryUtility.m_scansRequested.store(scans);
        m_TelemetryUtility.m_scanErrors.store(errors);
    }
};


TEST_F(TestOnAccessTelemetryUtility, InitialisesToZeroValues)
{
    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 0);
}

TEST_F(TestOnAccessTelemetryUtility, IncrementsScanEvent)
{
    m_TelemetryUtility.incrementFilesScanned(false);

    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), 1);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 0);
}

TEST_F(TestOnAccessTelemetryUtility, IncrementsScanEventAndError)
{
    m_TelemetryUtility.incrementFilesScanned(true);

    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), 1);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 1);
}

TEST_F(TestOnAccessTelemetryUtility, IncrementsEventReceived)
{
    m_TelemetryUtility.incrementEventReceived(false);

    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), 1);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 0);
}

TEST_F(TestOnAccessTelemetryUtility, IncrementsEventReceivedAndDropped)
{
    m_TelemetryUtility.incrementEventReceived(true);

    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), 1);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 1);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 0);
}


TEST_F(TestOnAccessTelemetryUtility, StoresValuesCorrectly)
{
    const uint eventTotal = 10;
    const uint eventDropped = 6;
    const uint scanTotal = 11;
    const uint scanError = 7;

    initialiseTelemetryWithValues(eventDropped, eventTotal, scanError, scanTotal);

    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), eventTotal);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), eventDropped);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), scanTotal);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), scanError);
}

TEST_F(TestOnAccessTelemetryUtility, NoScanErrorsOrDroppedEvents)
{
    const float expectedEventsDroppedPer = 0.0f;
    const float expectedScanErrorsPer = 0.0f;

    initialiseTelemetryWithValues(0, 10, 0, 10);

    auto result = m_TelemetryUtility.getTelemetry();

    EXPECT_EQ(result.m_percentageEventsDropped, expectedEventsDroppedPer);
    EXPECT_EQ(result.m_percentageScanErrors, expectedScanErrorsPer);
}

TEST_F(TestOnAccessTelemetryUtility, AllScanErrorsOrDroppedEvents)
{
    const float expectedEventsDroppedPer = 100.0f;
    const float expectedScanErrorsPer = 100.0f;

    initialiseTelemetryWithValues(10, 10, 10, 10);

    auto result = m_TelemetryUtility.getTelemetry();

    EXPECT_EQ(result.m_percentageEventsDropped, expectedEventsDroppedPer);
    EXPECT_EQ(result.m_percentageScanErrors, expectedScanErrorsPer);
}

TEST_F(TestOnAccessTelemetryUtility, NoEventsAndNoScans)
{
    const float expectedEventsDroppedPer = 0.0f;
    const float expectedScanErrorsPer = 0.0f;

    initialiseTelemetryWithValues(0, 0, 0, 0);

    auto result = m_TelemetryUtility.getTelemetry();

    EXPECT_EQ(result.m_percentageEventsDropped, expectedEventsDroppedPer);
    EXPECT_EQ(result.m_percentageScanErrors, expectedScanErrorsPer);
}

TEST_F(TestOnAccessTelemetryUtility, HandlesPercentageCalculation)
{
    const float expectedEventsDroppedPer = 50.0f;
    const float expectedScanErrorsPer = 50.0f;

    initialiseTelemetryWithValues(5, 10, 5, 10);

    auto result1 = m_TelemetryUtility.getTelemetry();

    EXPECT_EQ(result1.m_percentageEventsDropped, expectedEventsDroppedPer);
    EXPECT_EQ(result1.m_percentageScanErrors, expectedScanErrorsPer);
}


TEST_F(TestOnAccessTelemetryUtility, ResetsAfterGetTelemetry)
{
    const float expectedEventsDroppedPer = 50.0f;
    const float expectedScanErrorsPer = 50.0f;

    initialiseTelemetryWithValues(5, 10, 5, 10);

    auto result1 = m_TelemetryUtility.getTelemetry();

    EXPECT_EQ(result1.m_percentageEventsDropped, expectedEventsDroppedPer);
    EXPECT_EQ(result1.m_percentageScanErrors, expectedScanErrorsPer);

    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 0);
}

TEST_F(TestOnAccessTelemetryUtility, HandlesExtremeValues)
{
    const float expectedEventsDroppedPer = 5.42101e-17f;
    const float expectedScanErrorsPer = 0.00152586f;

    initialiseTelemetryWithValues(10, std::numeric_limits<unsigned long>::max(), std::numeric_limits<unsigned short>::max(), std::numeric_limits<unsigned int>::max());

    auto result = m_TelemetryUtility.getTelemetry();

    EXPECT_TRUE(fabs(result.m_percentageEventsDropped - expectedEventsDroppedPer) < 1e-22f);
    EXPECT_TRUE(fabs(result.m_percentageScanErrors - expectedScanErrorsPer) < 0.00000001f);
}

TEST_F(TestOnAccessTelemetryUtility, HandlesIncrementingBeyondLimitEvents)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    initialiseTelemetryWithValues(0, std::numeric_limits<unsigned long>::max(), 0, 0);

    ASSERT_EQ(m_TelemetryUtility.m_eventsReceived.load(), std::numeric_limits<unsigned long>::max());

    m_TelemetryUtility.incrementFilesScanned(true);
    m_TelemetryUtility.incrementEventReceived(true);

    EXPECT_TRUE(waitForLog("A Telemetry Value for Events at limit"));

    //Neither event data has incremented
    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), std::numeric_limits<unsigned long>::max());
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 0);

    //We can still increment scan data
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), 1);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 1);
}

TEST_F(TestOnAccessTelemetryUtility, HandlesIncrementingBeyondLimitScans)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    initialiseTelemetryWithValues(0, 0, 0, std::numeric_limits<unsigned long>::max());

    ASSERT_EQ(m_TelemetryUtility.m_scansRequested.load(), std::numeric_limits<unsigned long>::max());

    m_TelemetryUtility.incrementFilesScanned(true);
    m_TelemetryUtility.incrementEventReceived(true);

    EXPECT_TRUE(waitForLog("A Telemetry Value for Scans at limit"));

    //Neither scan data has incremented
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), std::numeric_limits<unsigned long>::max());
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 0);

    //We can still increment event data
    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), 1);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 1);
}

TEST_F(TestOnAccessTelemetryUtility, HandlesOnlyLogsOnceIfAboveLimitEventsReceived)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    initialiseTelemetryWithValues(0, std::numeric_limits<unsigned long>::max(), 0, 0);

    ASSERT_EQ(m_TelemetryUtility.m_eventsReceived.load(), std::numeric_limits<unsigned long>::max());

    m_TelemetryUtility.incrementEventReceived(false);

    EXPECT_TRUE(waitForLog("A Telemetry Value for Events at limit"));

    clearMemoryAppender();
    m_TelemetryUtility.incrementEventReceived(false);

    //Check to make sure the above has been processed.
    m_TelemetryUtility.incrementFilesScanned(false);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested, 1);

    EXPECT_FALSE(appenderContains("A Telemetry Value for Events at limit"));
}

TEST_F(TestOnAccessTelemetryUtility, HandlesOnlyLogsOnceIfAboveLimitEventsDropped)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    initialiseTelemetryWithValues(std::numeric_limits<unsigned int>::max(), std::numeric_limits<unsigned int>::max() + 1, 0, 0);

    ASSERT_EQ(m_TelemetryUtility.m_eventsDropped.load(), std::numeric_limits<unsigned int>::max());

    m_TelemetryUtility.incrementEventReceived(true);

    EXPECT_TRUE(waitForLog("A Telemetry Value for Events at limit"));

    clearMemoryAppender();
    m_TelemetryUtility.incrementEventReceived(true);

    //Check to make sure the above has been processed.
    m_TelemetryUtility.incrementFilesScanned(false);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested, 1);

    EXPECT_FALSE(appenderContains("A Telemetry Value for Events at limit"));
}


TEST_F(TestOnAccessTelemetryUtility, HandlesOnlyLogsOnceIfAboveLimitScansMade)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    initialiseTelemetryWithValues(0, 0, 0, std::numeric_limits<unsigned long>::max());

    ASSERT_EQ(m_TelemetryUtility.m_scansRequested.load(), std::numeric_limits<unsigned long>::max());

    m_TelemetryUtility.incrementFilesScanned(false);

    EXPECT_TRUE(waitForLog("A Telemetry Value for Scans at limit"));

    clearMemoryAppender();
    m_TelemetryUtility.incrementFilesScanned(false);

    //Check to make sure the above has been processed.
    m_TelemetryUtility.incrementEventReceived(false);
    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived, 1);

    EXPECT_FALSE(appenderContains("A Telemetry Value for Events at limit"));
}

TEST_F(TestOnAccessTelemetryUtility, HandlesOnlyLogsOnceIfAboveLimitScansError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    initialiseTelemetryWithValues(0, 0, std::numeric_limits<unsigned int>::max(), std::numeric_limits<unsigned int>::max() + 1);

    ASSERT_EQ(m_TelemetryUtility.m_scanErrors.load(), std::numeric_limits<unsigned int>::max());

    m_TelemetryUtility.incrementFilesScanned(true);

    EXPECT_TRUE(waitForLog("A Telemetry Value for Scans at limit"));

    clearMemoryAppender();
    m_TelemetryUtility.incrementFilesScanned(true);

    //Check to make sure the above has been processed.
    m_TelemetryUtility.incrementEventReceived(false);
    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived, 1);

    EXPECT_FALSE(appenderContains("A Telemetry Value for Events at limit"));
}
