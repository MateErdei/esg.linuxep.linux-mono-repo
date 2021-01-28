/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "InstallStateMachine.h"

namespace stateMachinesModule
{
    InstallStateMachine::InstallStateMachine(const StateData::InstallMachineState& state, const std::chrono::system_clock::time_point& now) :
        state_{ state }
    {
        if (StateData::InstallMachineState::defaultCredit == state_.credit)
        {
            if (std::chrono::system_clock::time_point{} == state_.lastGood)
            {
                state_.lastGood = now;
            }
        }
        else
        {
            if (std::chrono::system_clock::time_point{} == state_.failedSince)
            {
                state_.failedSince = now;
            }
        }
    }

    void InstallStateMachine::SignalInstallResult(
        StateData::StatusEnum resultStatus,
        const std::chrono::system_clock::time_point& now)
    {
        switch (resultStatus)
        {
        case StateData::StatusEnum::good:
            state_.credit = StateData::InstallMachineState::defaultCredit;
            state_.lastGood = now;
            state_.failedSince = {};
            break;
        case StateData::StatusEnum::bad:
            if (state_.credit > 0)
            {
                --state_.credit;
                if (std::chrono::system_clock::time_point{} == state_.failedSince)
                {
                    state_.failedSince = now;
                }
            }
            break;
        }
    }

    StateData::InstallMachineState InstallStateMachine::CurrentState() const
    {
        return state_;
    }
} // namespace stateMachinesModule
