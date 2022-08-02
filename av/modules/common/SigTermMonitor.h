// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "signals/SignalHandlerBase.h"

#include <memory>

namespace common
{
    class SigTermMonitor : public common::signals::SignalHandlerBase
    {
    public:
        explicit SigTermMonitor();
        ~SigTermMonitor() override;

        static std::shared_ptr<SigTermMonitor> getSigTermMonitor();
        bool triggered();

    };
}