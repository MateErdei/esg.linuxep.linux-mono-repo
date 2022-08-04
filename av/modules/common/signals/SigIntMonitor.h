// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "LatchingSignalHandler.h"

#include <memory>

namespace common::signals
{
    class SigIntMonitor : public common::signals::LatchingSignalHandler
    {
    public:
        explicit SigIntMonitor(bool restartSyscalls=true);
        ~SigIntMonitor() override;

        static std::shared_ptr<SigIntMonitor> getSigIntMonitor(bool restartSyscalls=true);

    };
}