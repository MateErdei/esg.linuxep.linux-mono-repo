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
                std::stringstream msg;
                msg << "usage:" << argv[0] << "<configuration file path>";
                throw std::runtime_error(msg.str());
            }

            std::string configFilePath(argv[1]);

            if (!Common::FileSystem::fileSystem()->isFile(configFilePath))
            {
                std::stringstream msg;
                msg << "Configuration file '" << configFilePath << "' is not accessible";
                throw std::runtime_error(msg.str());
            }

            std::string telemetryConfigJson = Common::FileSystem::fileSystem()->readFile(configFilePath, 1000000UL);

            auto config = std::make_shared<const TelemetryConfig::Config>(
                TelemetryConfig::TelemetryConfigSerialiser::deserialise(telemetryConfigJson));

            LOGDEBUG("Using configuration: " << TelemetryConfig::TelemetryConfigSerialiser::serialise(*config));

            std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper =
                std::make_shared<Common::HttpSenderImpl::CurlWrapper>();

            std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders;

            auto systemTelemetryReporter =
                std::make_shared<SystemTelemetryReporter>(std::make_unique<SystemTelemetryCollectorImpl>(
                    GL_systemTelemetryObjectsConfig,
                    GL_systemTelemetryArraysConfig,
                    config->getExternalProcessTimeout(),
                    config->getExternalProcessRetries()));

            telemetryProviders.emplace_back(systemTelemetryReporter);
            TelemetryProcessor telemetryProcessor(
                config, std::make_unique<Common::HttpSenderImpl::HttpSender>(curlWrapper), telemetryProviders);

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
