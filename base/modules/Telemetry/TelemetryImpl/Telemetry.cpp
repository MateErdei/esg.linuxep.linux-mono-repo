/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include "ISystemTelemetryCollector.h"
#include "SystemTelemetryCollectorImpl.h"
#include "SystemTelemetryReporter.h"
#include "TelemetryProcessor.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/HttpSenderImpl/HttpSender.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Telemetry/LoggerImpl/Logger.h>
#include <Telemetry/TelemetryConfigImpl/Config.h>
#include <Telemetry/TelemetryConfigImpl/TelemetryConfigSerialiser.h>

#include <json.hpp>
#include <sstream>
#include <string>

namespace Telemetry
{
    std::unique_ptr<TelemetryProcessor> initialiseTelemetryProcessor(
        std::shared_ptr<TelemetryConfigImpl::Config> telemetryConfig);

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
            auto telemetryConfig = std::make_shared<TelemetryConfigImpl::Config>(
                TelemetryConfigImpl::TelemetryConfigSerialiser::deserialise(telemetryConfigJson));
            LOGDEBUG("Using configuration: " << TelemetryConfigImpl::TelemetryConfigSerialiser::serialise(*telemetryConfig));

            std::unique_ptr<TelemetryProcessor> telemetryProcessor = initialiseTelemetryProcessor(telemetryConfig);

            telemetryProcessor->Run();

            return 0;
        }
        catch (const std::runtime_error& e)
        {
            LOGERROR("Caught runtime exception: " << e.what());
            return 1;
        }
        catch (const nlohmann::detail::parse_error& e)
        {
            LOGERROR("Caught JSON parse error: " << e.what());
            return 1;
        }
    }

    std::unique_ptr<TelemetryProcessor> initialiseTelemetryProcessor(
        std::shared_ptr<TelemetryConfigImpl::Config> telemetryConfig)
    {
        std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper =
            std::make_shared<Common::HttpSenderImpl::CurlWrapper>();

        std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders;

        auto systemTelemetryReporter =
            std::make_shared<SystemTelemetryReporter>(std::make_unique<SystemTelemetryCollectorImpl>(
                GL_systemTelemetryObjectsConfig,
                GL_systemTelemetryArraysConfig,
                telemetryConfig->getExternalProcessWaitTime(),
                telemetryConfig->getExternalProcessWaitRetries()));

        telemetryProviders.emplace_back(systemTelemetryReporter);

        std::unique_ptr<TelemetryProcessor> telemetryProcessor = std::make_unique<TelemetryProcessor>(
            telemetryConfig, std::make_unique<Common::HttpSenderImpl::HttpSender>(curlWrapper), telemetryProviders);

        return telemetryProcessor;
    }
} // namespace Telemetry
