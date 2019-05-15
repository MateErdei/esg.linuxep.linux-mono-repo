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

        virtual void Run();

    protected:
        virtual void gatherTelemetry();
        virtual std::string getSerialisedTelemetry();
        virtual void sendTelemetry(const std::string& telemetryJson);
        virtual void saveTelemetry(const std::string& telemetryJson) const;
        virtual void addTelemetry(const std::string& sourceName, const std::string& json);

    private:
        const TelemetryConfig::Config& m_config;
        Common::HttpSender::IHttpSender& m_httpSender;
        Common::Telemetry::TelemetryHelper m_telemetryHelper;
        std::vector<std::shared_ptr<ITelemetryProvider>> m_telemetryProviders;
    };
}
