/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "InstallStateMachine.h"

#include <climits>

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
                if (state_.credit <= INT_MAX)
                {
                    --state_.credit;
                }
                else // negative
                {
                    state_.credit = 0;
                }
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

    int InstallStateMachine::getOverallState() const
    {
        // (credit == 0) => bad
        // ((credit > 0) && (credit < defaultCredit)) => good (assuming temporary failure)
        // (credit == defaultCredit) => good
        if (state_.credit == 0)
        {
            return 1; // bad
        }
        else
        {
            return 0; // good
        }
    }
} // namespace stateMachinesModule
