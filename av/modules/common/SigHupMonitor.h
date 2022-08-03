// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "signals/LatchingSignalHandler.h"

#include <memory>

namespace common
{
    class SigHupMonitor : public common::signals::LatchingSignalHandler
    {
    public:
        explicit SigHupMonitor(bool restartSyscalls=true);
        ~SigHupMonitor() override;

        static std::shared_ptr<SigHupMonitor> getSigHupMonitor(bool restartSyscalls=true);

    };
}