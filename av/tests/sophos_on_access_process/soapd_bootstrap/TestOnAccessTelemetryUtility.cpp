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