// Copyright 2020 Sophos Limited.

#include "InstallComponentState.h"

namespace StateLib
{
    InstallComponentState::InstallComponentState(const ComponentState& componentState) :
        componentState_{ componentState}
    {}

    bool  InstallComponentState::is_downloaded()
    {
        return !componentState_.GetDownloadedThumbprint().GetThumbprint().empty();
    }

    bool  InstallComponentState::is_installed()
    {
        return is_downloaded() && (componentState_.GetDownloadedThumbprint() == componentState_.GetInstalledThumbprint());
    }

    bool  InstallComponentState::has_failed_to_install()
    {
        return is_downloaded() && (componentState_.GetInstalledThumbprint() == ComponentState::FailedInstallThumbprint());
    }

    bool  InstallComponentState::is_marked_for_reinstall()
    {
        return is_downloaded() && (componentState_.GetInstalledThumbprint().GetThumbprint().empty());
    }

    InstallComponentState GetInstallComponentState(const IState& state, const std::string& compId)
    {
        auto csm = state.GetComponentStateMap();
        // During an update statemaps may contain lineIds of components that are not be installed
        if (auto component = csm.find(compId); component != end(csm))
        {
            return InstallComponentState(component->second);
        }
        return InstallComponentState(ComponentState{});
    }
} // namespace StateLib
