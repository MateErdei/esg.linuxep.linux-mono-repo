/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "StateData.h"

namespace stateMachinesModule
{
    class InstallStateMachine
    {
        StateData::InstallMachineState state_;

    public:
        InstallStateMachine(const StateData::InstallMachineState& state, const std::chrono::system_clock::time_point& now);

        void SignalInstallResult(
            StateData::StatusEnum resultStatus,
            const std::chrono::system_clock::time_point& now);

        StateData::InstallMachineState CurrentState() const;
    };
} // namespace stateMachinesModule
