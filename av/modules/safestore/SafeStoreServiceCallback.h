// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

namespace safestore
{
    class SafeStoreServiceCallback : public Common::PluginApi::IPluginCallbackApi
    {
    public:
        std::string getTelemetry() override;

    private:
        void applyNewPolicy(const std::string& policyXml) override;
        std::string getHealth() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appid) override;
        void onShutdown() override {}
        void queueAction(const std::string& action) override;

        [[nodiscard]] std::optional<unsigned long> getSafeStoreDatabaseSize() const;
    };

} // namespace safestore
