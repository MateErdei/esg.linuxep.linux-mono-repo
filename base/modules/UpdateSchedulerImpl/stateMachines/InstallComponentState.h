// Copyright 2020 Sophos Limited.

#pragma once

#include "UpdateScheduler/IState.h"
#include "ComponentState.h"

namespace StateLib
{
    class InstallComponentState
    {
        ComponentState componentState_;
    public:
        explicit InstallComponentState(const ComponentState& componentState);
        bool is_downloaded();
        bool is_installed();
        bool has_failed_to_install();
        bool is_marked_for_reinstall();
    };

    InstallComponentState GetInstallComponentState(const IState& state, const std::string& lineId);

} // namespace StateLib

