/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITelemetryProvider.h"
#include "../TelemetryConfig/Config.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/HttpSender/IHttpSender.h>
#include <Common/HttpSenderImpl/RequestConfig.h>

#include <utility>

using namespace Common::Telemetry;

namespace Telemetry
{
    class TelemetryProcessor
    {
    public:
        TelemetryProcessor(
            const TelemetryConfig::Config& config,
            Common::HttpSender::IHttpSender& httpSender,
            std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders);

        void Run();

    public: // TODO: consider making these private or protected
        void gatherTelemetry();
        std::string getSerialisedTelemetry();
        void sendTelemetry(const std::string& telemetryJson);
        void saveTelemetry(const std::string& telemetryJson) const;

    private:
        const TelemetryConfig::Config& m_config;
        Common::HttpSender::IHttpSender& m_httpSender;
        Common::Telemetry::TelemetryHelper m_telemetryHelper;
        std::vector<std::shared_ptr<ITelemetryProvider>> m_telemetryProviders;

        void addTelemetry(const std::string& sourceName, const std::string& json);
    };
}
