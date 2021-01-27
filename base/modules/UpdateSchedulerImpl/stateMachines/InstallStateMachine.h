// Copyright 2020 Sophos Limited.

#pragma once

//#include "UpdateScheduler/IEnvironment.h"
#include "StateData.h"   //move in interface directory?

namespace StateLib
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
} // namespace StateLib
