/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventStateMachine.h"

#include "SEUErrors.h"

namespace stateMachinesModule
{
    bool EventStateMachine::DiscardTemporaryError(int updateError) const
    {
        switch (updateError)
        {
        case 0:
        case IDS_RMS_REBOOTREQUIRED:
            // Success.
            return false;
        case IDS_RMS_DWNLD_ERROR:
        case IDS_RMS_CONNECTIONFAILED:
            // Download failure.
            return 0 != downloadStateMachine_.CurrentState().credit;
        default:
            // Install failure.
            return 0 != installStateMachine_.CurrentState().credit;
        }
    }

    bool EventStateMachine::DiscardRecentDuplicate(int updateError, const std::chrono::system_clock::time_point& now) const
    {
        return (updateError == state_.lastError) && (now - state_.lastTime < StateData::EventMachineState::throttlePeriod);
    }

    EventStateMachine::EventStateMachine(DownloadStateMachine& downloadStateMachine, InstallStateMachine& installStateMachine, const StateData::EventMachineState& state) :
        downloadStateMachine_{ downloadStateMachine },
        installStateMachine_{ installStateMachine },
        state_{ state }
    {
    }

    void EventStateMachine::Reset(const StateData::EventMachineState& state)
    {
        state_ = state;
    }

    bool EventStateMachine::Discard(int updateError, const std::chrono::system_clock::time_point& now)
    {
        if (DiscardTemporaryError(updateError))
        {
            return true;
        }

        if (DiscardRecentDuplicate(updateError, now))
        {
            return true;
        }

        state_.lastError = updateError;
        state_.lastTime = now;
        return false;
    }

    StateData::EventMachineState EventStateMachine::CurrentState() const
    {
        return state_;
    }
} // namespace stateMachinesModule
