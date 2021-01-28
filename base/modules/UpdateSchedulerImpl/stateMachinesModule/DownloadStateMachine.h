/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "StateData.h"

namespace stateMachinesModule
{
    class DownloadStateMachine
    {
        StateData::DownloadMachineState state_;

        void HandleFailure(const std::chrono::system_clock::time_point& now, int decrement);

    public:
        DownloadStateMachine(const StateData::DownloadMachineState& state, const std::chrono::system_clock::time_point& now);

        void SignalDownloadResult(
            StateData::DownloadResultEnum downloadResult,
            const std::chrono::system_clock::time_point& now);

        StateData::DownloadMachineState CurrentState() const;
    };
} // namespace stateMachinesModule
