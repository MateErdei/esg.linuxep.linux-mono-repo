// Copyright 2020 Sophos Limited.

#include "RebootStateMachine.h"

namespace StateLib
{
    RebootStateMachine::RebootStateMachine(const RebootData::MachineState& state) :
        state_{ state }
    {
    }

    void RebootStateMachine::SignalRebootRequired(RebootData::RequiredEnum required, const std::chrono::system_clock::time_point& now)
    {
        if (state_.requiredEnum < required)
        {
            if (state_.requiredEnum == RebootData::RequiredEnum::RebootNotRequired)
            {
                state_.requiredSince = now;
            }
            state_.requiredEnum = required;
        }
    }

    void RebootStateMachine::SignalScheduleTick()
    {
        switch (state_.requiredEnum)
        {
        case RebootData::RequiredEnum::RebootOptional:
        case RebootData::RequiredEnum::RebootMandatory:
            if (state_.credit > 0)
            {
                --state_.credit;
            }
            break;
        case RebootData::RequiredEnum::RebootNotRequired:
            break;
        }
    }

    RebootData::MachineState RebootStateMachine::CurrentState() const
    {
        return state_;
    }
} // namespace StateLib
