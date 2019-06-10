/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITelemetryProvider.h"

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/HttpSender/IHttpSender.h>
#include <Common/HttpSenderImpl/RequestConfig.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryConfigImpl/Config.h>

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
            std::unique_ptr<Common::HttpSender::IHttpSender> httpSender,
            std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders);

        virtual void Run();

    protected:
        virtual void gatherTelemetry();
        virtual std::string getSerialisedTelemetry();
        virtual void sendTelemetry(const std::string& telemetryJson);
        virtual void saveTelemetry(const std::string& telemetryJson) const;
        virtual void addTelemetry(const std::string& sourceName, const std::string& json);

    private:
        std::shared_ptr<const Config> m_config;
        std::unique_ptr<Common::HttpSender::IHttpSender> m_httpSender;
        Common::Telemetry::TelemetryHelper m_telemetryHelper;
        std::vector<std::shared_ptr<ITelemetryProvider>> m_telemetryProviders;
    };
} // namespace Telemetry
