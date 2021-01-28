/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadStateMachine.h"

#include <algorithm>

namespace stateMachinesModule
{
    void DownloadStateMachine::HandleFailure(const std::chrono::system_clock::time_point& now, int decrement)
    {
        if (0 == state_.credit)
        {
            return;
        }

        if (std::chrono::system_clock::time_point{} == state_.failedSince)
        {
            state_.failedSince = now;
        }

        state_.credit -= std::min<int>(state_.credit, decrement);
    }

    DownloadStateMachine::DownloadStateMachine(const StateData::DownloadMachineState& state, const std::chrono::system_clock::time_point& now) :
        state_{ state }
    {
        if (StateData::DownloadMachineState::defaultCredit != state_.credit)
        {
            if (std::chrono::system_clock::time_point{} == state_.failedSince)
            {
                state_.failedSince = now;
            }
        }
    }

    void DownloadStateMachine::SignalDownloadResult(
        StateData::DownloadResultEnum downloadResult,
        const std::chrono::system_clock::time_point& now)
    {
        switch (downloadResult)
        {
        case StateData::DownloadResultEnum::good:
            state_.credit = StateData::DownloadMachineState::defaultCredit;
            state_.failedSince = {};
            break;
        case StateData::DownloadResultEnum::bad:
            HandleFailure(now, StateData::DownloadMachineState::noNetworkCreditMultiplicationFactor);
            break;
        case StateData::DownloadResultEnum::noNetwork:
            HandleFailure(now, 1);
            break;
        }
    }

    StateData::DownloadMachineState DownloadStateMachine::CurrentState() const
    {
        return state_;
    }
} // namespace stateMachinesModule
