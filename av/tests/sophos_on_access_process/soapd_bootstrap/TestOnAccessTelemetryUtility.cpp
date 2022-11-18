// Copyright 2022, Sophos Limited.  All rights reserved.

# define TEST_PUBLIC public

#include "sophos_on_access_process/soapd_bootstrap/OnAccessTelemetryUtility.h"

#include "common/MemoryAppender.h"

#include <gtest/gtest.h>

using namespace sophos_on_access_process::OnAccessTelemetry;
using namespace testing;


class TestOnAccessTelemetryUtility : public MemoryAppenderUsingTests
{
protected:
    TestOnAccessTelemetryUtility() : MemoryAppenderUsingTests("soapd_bootstrap") {}

    OnAccessTelemetryUtility m_TelemetryUtility;

    void populateTelemetryUtility(uint64_t eventsDropped, uint64_t eventsTotal, uint64_t scanError, uint64_t scanTotal)
    {
        uint64_t eventDroppedCount = 0;
        for (uint64_t i = 0; i < eventsTotal; i++)
        {
            bool dropped = false;
            if (eventDroppedCount < eventsDropped)
            {
                dropped = true;
            }
            eventDroppedCount++;
            m_TelemetryUtility.incrementEventReceived(dropped);
        }

        uint64_t scanErrorCount = 0;
        for (uint64_t i = 0; i < scanTotal; i++)
        {
            bool error = false;
            if (scanErrorCount < scanError)
            {
                error = true;
            }
            scanErrorCount++;
            m_TelemetryUtility.incrementFilesScanned(error);
        }
    }
};


TEST_F(TestOnAccessTelemetryUtility, InitialisesToZeroValues)
{
    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), 0);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), 0);
}

TEST_F(TestOnAccessTelemetryUtility, StoresValuesCorrectly)
{
    const uint eventTotal = 10;
    const uint eventDropped = 5;
    const uint scanTotal = 10;
    const uint scanError = 5;

    populateTelemetryUtility(eventDropped, eventTotal, scanError, scanTotal);

    EXPECT_EQ(m_TelemetryUtility.m_eventsReceived.load(), eventTotal);
    EXPECT_EQ(m_TelemetryUtility.m_eventsDropped.load(), eventDropped);
    EXPECT_EQ(m_TelemetryUtility.m_scansRequested.load(), scanTotal);
    EXPECT_EQ(m_TelemetryUtility.m_scanErrors.load(), scanError);
}

TEST_F(TestOnAccessTelemetryUtility, NoErrors)
{
    const float expectedEventsDroppedPer = 0.0f;
    const float expectedScanErrorsPer = 0.0f;

    populateTelemetryUtility(0, 10, 0, 10);

    auto result = m_TelemetryUtility.getTelemetry();

    ASSERT_EQ(result.m_percentageEventsDropped, expectedEventsDroppedPer);
    ASSERT_EQ(result.m_percentageScanErrors, expectedScanErrorsPer);
}

TEST_F(TestOnAccessTelemetryUtility, AllErrors)
{
    const float expectedEventsDroppedPer = 100.0f;
    const float expectedScanErrorsPer = 100.0f;

    populateTelemetryUtility(10, 10, 10, 10);

    auto result = m_TelemetryUtility.getTelemetry();

    ASSERT_EQ(result.m_percentageEventsDropped, expectedEventsDroppedPer);
    ASSERT_EQ(result.m_percentageScanErrors, expectedScanErrorsPer);
}

TEST_F(TestOnAccessTelemetryUtility, AllZeros)
{
    const float expectedEventsDroppedPer = 0.0f;
    const float expectedScanErrorsPer = 0.0f;

    populateTelemetryUtility(0, 0, 0, 0);

    auto result = m_TelemetryUtility.getTelemetry();

    ASSERT_EQ(result.m_percentageEventsDropped, expectedEventsDroppedPer);
    ASSERT_EQ(result.m_percentageScanErrors, expectedScanErrorsPer);
}