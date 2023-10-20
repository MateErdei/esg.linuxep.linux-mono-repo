// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_on_access_process/IOnAccessTelemetryUtility/IOnAccessTelemetryUtility.h"

namespace sophos_on_access_process::service_impl
{
    class IOnAccessService
    {
    public:
        virtual ~IOnAccessService() = default;

        virtual onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr getTelemetryUtility() = 0;
    };

    using IOnAccessServicePtr = std::unique_ptr<IOnAccessService>;

} // namespace sophos_on_access_process::service_impl