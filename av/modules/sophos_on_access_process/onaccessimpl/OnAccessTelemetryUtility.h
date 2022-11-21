// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "OnAccessTelemetryFields.h"

#include <cstdint>

#include <atomic>
#include <memory>

namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry
{
    class OnAccessTelemetryUtility
    {
    public:
        struct TelemetryEntry
        {
            float m_percentageEventsDropped = 0.0;
            float m_percentageScanErrors = 0.0;
        };

        TelemetryEntry getTelemetry();
        void incrementEventReceived(bool dropped);
        void incrementFilesScanned(bool error);

        OnAccessTelemetryUtility();
        OnAccessTelemetryUtility(const OnAccessTelemetryUtility&) = delete;
        OnAccessTelemetryUtility& operator=(const OnAccessTelemetryUtility&) = delete;
        OnAccessTelemetryUtility(OnAccessTelemetryUtility&&) = delete;
        OnAccessTelemetryUtility& operator=(OnAccessTelemetryUtility&&) = delete;

    private:
        float calculatePerEventsDropped();
        float calculatePerScanErrors();
        float calculatePercentage(uint64_t total, uint32_t problems);

        std::atomic_bool m_eventsAtLimit = false;
        std::atomic_bool m_scansAtLimit = false;

    TEST_PUBLIC:
        std::atomic_uint64_t m_eventsReceived;
        std::atomic_uint32_t m_eventsDropped;
        std::atomic_uint64_t m_scansRequested;
        std::atomic_uint32_t m_scanErrors;
    };
    using OnAccessTelemetryUtilitySharedPtr = std::shared_ptr<OnAccessTelemetryUtility>;
}
