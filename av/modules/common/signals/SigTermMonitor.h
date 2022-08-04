// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "LatchingSignalHandler.h"

#include <memory>

namespace common
{
    class SigTermMonitor : public common::signals::LatchingSignalHandler
    {
    public:
        explicit SigTermMonitor(bool restartSyscalls);
        ~SigTermMonitor() override;

        static std::shared_ptr<SigTermMonitor> getSigTermMonitor(bool restartSyscalls);

    };
}