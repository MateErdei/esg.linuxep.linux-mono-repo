// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

namespace sophos_on_access_process::service_callback
{
    class OnAccessServiceCallback : public Common::PluginApi::IPluginCallbackApi
    {
    public:
        std::string getTelemetry() override;

    private:
        //The below are not used by OnAccess
        void applyNewPolicy(const std::string& policyXml) override;
        std::string getHealth() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appid) override;
        void onShutdown() override {}
        void queueAction(const std::string& action) override;
    };
}


