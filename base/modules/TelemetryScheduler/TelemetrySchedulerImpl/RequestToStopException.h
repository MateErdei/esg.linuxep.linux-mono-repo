// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace TelemetrySchedulerImpl
{
    class RequestToStopException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };
} // namespace TelemetrySchedulerImpl
