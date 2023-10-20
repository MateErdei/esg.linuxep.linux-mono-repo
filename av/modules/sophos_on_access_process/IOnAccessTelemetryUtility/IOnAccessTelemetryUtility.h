// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry
{
    class IOnAccessTelemetryUtility
    {
    public:
        struct TelemetryEntry
        {
            float m_percentageEventsDropped = 0.0;
            float m_percentageScanErrors = 0.0;
        };

        virtual TelemetryEntry getTelemetry() = 0;
        virtual void incrementEventReceived(bool dropped) = 0;
        virtual void incrementFilesScanned(bool error) = 0;

        IOnAccessTelemetryUtility() = default;
        virtual ~IOnAccessTelemetryUtility() = default;
        IOnAccessTelemetryUtility(const IOnAccessTelemetryUtility&) = delete;
        IOnAccessTelemetryUtility& operator=(const IOnAccessTelemetryUtility&) = delete;
        IOnAccessTelemetryUtility(IOnAccessTelemetryUtility&&) = delete;
        IOnAccessTelemetryUtility& operator=(IOnAccessTelemetryUtility&&) = delete;

    };

    using IOnAccessTelemetryUtilitySharedPtr = std::shared_ptr<IOnAccessTelemetryUtility>;
}
