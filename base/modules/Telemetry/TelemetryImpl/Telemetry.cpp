/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include "ISystemTelemetryCollector.h"
#include "SystemTelemetryCollectorImpl.h"
#include "SystemTelemetryReporter.h"
#include "TelemetryProcessor.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Telemetry/LoggerImpl/Logger.h>
#include <Telemetry/TelemetryConfig/Config.h>
#include <Telemetry/TelemetryConfig/TelemetryConfigSerialiser.h>

#include <sstream>
#include <string>

namespace Telemetry
{
    int main_entry(int argc, char* argv[])
    {
        Common::Logging::FileLoggingSetup loggerSetup("telemetry", true);

        try
        {
            if (argc != 2)
            {
                throw std::runtime_error("usage: argv[0] <configuration file path>");
            }

            if (!Common::FileSystem::fileSystem()->isFile(argv[1]))
            {
                throw std::runtime_error("Configuration file is not accessible");
            }

            std::string telemetryConfigJson = Common::FileSystem::fileSystem()->readFile(argv[1], 1000000UL);

            TelemetryConfig::Config config = TelemetryConfig::TelemetryConfigSerialiser::deserialise(telemetryConfigJson);

            std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper =
                std::make_shared<Common::HttpSenderImpl::CurlWrapper>();
            Common::HttpSenderImpl::HttpSender httpSender(curlWrapper);

            std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders;

            auto systemTelemetryReporter =
                std::make_shared<SystemTelemetryReporter>(std::make_unique<SystemTelemetryCollectorImpl>(
                    GL_systemTelemetryObjectsConfig,
                    GL_systemTelemetryArraysConfig,
                    config.m_externalProcessTimeout,
                    config.m_externalProcessRetries));

            telemetryProviders.emplace_back(systemTelemetryReporter);
            TelemetryProcessor telemetryProcessor(config, httpSender, telemetryProviders);

            telemetryProcessor.Run();

            return 0;
        }
        catch (const std::runtime_error& e)
        {
            LOGERROR("Caught exception: " << e.what());
            return 1;
        }
    }
} // namespace Telemetry
