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
        explicit TelemetryProcessor(
            const TelemetryConfig::Config& config,
            std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders);
        void gatherTelemetry();
        void saveAndSendTelemetry(Common::HttpSender::IHttpSender& httpSender);
        std::string getSerialisedTelemetry();

    private:
        const TelemetryConfig::Config& m_config;
        Common::Telemetry::TelemetryHelper m_telemetryHelper;
        std::vector<std::shared_ptr<ITelemetryProvider>> m_telemetryProviders;

        void addTelemetry(const std::string& sourceName, const std::string& json);
    };

}