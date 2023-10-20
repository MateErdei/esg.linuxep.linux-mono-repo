// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_on_access_process/IOnAccessTelemetryUtility/IOnAccessTelemetryUtility.h"

#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

namespace sophos_on_access_process::service_callback
{
    class OnAccessServiceCallback : public Common::PluginApi::IPluginCallbackApi
    {
    public:
        explicit OnAccessServiceCallback(onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr telemetryUtility);
        std::string getTelemetry() override;

    private:
        onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr m_telemetryUtility = nullptr;

        //The below are not used by OnAccess
        void applyNewPolicy(const std::string& policyXml) override;
        std::string getHealth() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appid) override;
        void onShutdown() override {}
        void queueAction(const std::string& action) override;
    };
}


