// Copyright 2020 Sophos Limited.

#pragma once

#include "RebootData.h"

namespace StateLib
{
    class RebootStateMachine
    {
        RebootData::MachineState state_;

    public:
        explicit RebootStateMachine(const RebootData::MachineState& state);

        void SignalRebootRequired(RebootData::RequiredEnum required, const std::chrono::system_clock::time_point& now);
        void SignalScheduleTick();

        RebootData::MachineState CurrentState() const;
    };
} // namespace StateLib
