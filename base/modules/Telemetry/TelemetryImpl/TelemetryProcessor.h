// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ITelemetryProvider.h"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/TelemetryConfigImpl/Config.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <chrono>
#include <utility>

using namespace Common::Telemetry;
using namespace Common::TelemetryConfigImpl;

namespace Telemetry
{
    class TelemetryProcessor
    {
    public:
        TelemetryProcessor(
            std::shared_ptr<const Config> config,
            std::shared_ptr<Common::HttpRequests::IHttpRequester> httpRequester,
            std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders);
        virtual ~TelemetryProcessor() = default;

        virtual void Run();

    protected:
        virtual void gatherTelemetry();
        virtual std::string getSerialisedTelemetry();
        virtual void sendTelemetry(const std::string& telemetryJson);
        virtual void saveTelemetry(const std::string& telemetryJson) const;
        virtual void addTelemetry(const std::string& sourceName, const std::string& json);
        void updateStatusFile(const std::chrono::system_clock::time_point& lastRunTime) const;

    private:
        Common::HttpRequests::Response doHTTPCall(
            const std::string& verb,
            Common::HttpRequests::RequestConfig& request);
        std::shared_ptr<const Config> m_config;
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_httpRequester;
        Common::Telemetry::TelemetryHelper m_telemetryHelper;
        std::vector<std::shared_ptr<ITelemetryProvider>> m_telemetryProviders;
    };
} // namespace Telemetry
