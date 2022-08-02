// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "signals/SignalHandlerBase.h"

#include <memory>

namespace common
{
    class SigHupMonitor : public common::signals::SignalHandlerBase
    {
    public:
        explicit SigHupMonitor();
        ~SigHupMonitor() override;

        static std::shared_ptr<SigHupMonitor> getSigHupMonitor();
        bool triggered();

    };
}