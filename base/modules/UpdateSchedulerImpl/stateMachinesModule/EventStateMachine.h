/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "DownloadStateMachine.h"
#include "InstallStateMachine.h"

#include <optional>

namespace stateMachinesModule
{
    class EventStateMachine
    {
        DownloadStateMachine& downloadStateMachine_;
        InstallStateMachine& installStateMachine_;
        StateData::EventMachineState state_;

        bool DiscardTemporaryError(int updateError) const;
        bool DiscardRecentDuplicate(int updateError, const std::chrono::system_clock::time_point& now) const;

    public:
        EventStateMachine(DownloadStateMachine& downloadStateMachine, InstallStateMachine& installStateMachine, const StateData::EventMachineState& state);
        void Reset(const StateData::EventMachineState& state);

        bool Discard(int updateError, const std::chrono::system_clock::time_point& now);

        StateData::EventMachineState CurrentState() const;
    };
} // namespace stateMachinesModule
