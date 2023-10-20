// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "sophos_on_access_process/IOnAccessTelemetryUtility/IOnAccessTelemetryUtility.h"

#include <atomic>
#include <memory>

namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry
{
    class OnAccessTelemetryUtility : public IOnAccessTelemetryUtility
    {
    public:
        TelemetryEntry getTelemetry() override;
        void incrementEventReceived(bool dropped) override;
        void incrementFilesScanned(bool error) override;

        OnAccessTelemetryUtility() = default;
        OnAccessTelemetryUtility(const OnAccessTelemetryUtility&) = delete;
        OnAccessTelemetryUtility& operator=(const OnAccessTelemetryUtility&) = delete;
        OnAccessTelemetryUtility(OnAccessTelemetryUtility&&) = delete;
        OnAccessTelemetryUtility& operator=(OnAccessTelemetryUtility&&) = delete;

    private:
        float calculatePerEventsDropped();
        float calculatePerScanErrors();
        float calculatePercentage(unsigned long total, unsigned int problems);

        std::atomic_bool m_eventsAtLimit{ false };
        std::atomic_bool m_scansAtLimit{ false };

    TEST_PUBLIC:
        std::atomic_ulong m_eventsReceived { 0 };
        std::atomic_uint m_eventsDropped { 0 };
        std::atomic_ulong m_scansRequested { 0 };
        std::atomic_uint m_scanErrors { 0 };
    };

    using OnAccessTelemetryUtilitySharedPtr = std::shared_ptr<OnAccessTelemetryUtility>;
}
