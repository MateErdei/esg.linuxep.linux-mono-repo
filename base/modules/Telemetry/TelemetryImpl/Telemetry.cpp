/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include "ISystemTelemetryCollector.h"
#include "PluginTelemetryReporter.h"
#include "SystemTelemetryCollectorImpl.h"
#include "SystemTelemetryReporter.h"
#include "TelemetryProcessor.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/HttpSenderImpl/HttpSender.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/TelemetryConfigImpl/Config.h>
#include <Common/TelemetryConfigImpl/Serialiser.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <sstream>
#include <string>

namespace Telemetry
{
    void appendTelemetryProvidersForPlugins(std::vector<std::shared_ptr<ITelemetryProvider>>& telemetryProviders)
    {
        auto pluginManager = std::make_shared<ManagementAgent::PluginCommunicationImpl::PluginManager>(
            Common::ApplicationConfiguration::applicationPathManager().getTelemetrySocketAddress());

        std::vector<Common::PluginRegistryImpl::PluginInfo> plugins =
            Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

        for (auto& plugin : plugins)
        {
            pluginManager->registerAndSetAppIds(
                plugin.getPluginName(), plugin.getPolicyAppIds(), plugin.getStatusAppIds());
            LOGINFO("Registered plugin: " << plugin.getPluginName());

            auto telemetryProvider = std::make_shared<PluginTelemetryReporter>(pluginManager, plugin.getPluginName());
            telemetryProviders.emplace_back(telemetryProvider);
        }
    }

    std::unique_ptr<TelemetryProcessor> initialiseTelemetryProcessor(
        std::shared_ptr<Common::TelemetryConfigImpl::Config> telemetryConfig)
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
        appendTelemetryProvidersForPlugins(telemetryProviders);

        std::unique_ptr<TelemetryProcessor> telemetryProcessor = std::make_unique<TelemetryProcessor>(
            telemetryConfig, std::make_unique<Common::HttpSenderImpl::HttpSender>(curlWrapper), telemetryProviders);

        return telemetryProcessor;
    }

    int main_entry(int argc, char* argv[])
    {
        Common::Logging::FileLoggingSetup loggerSetup("telemetry", true);

        try
        {
            if (argc != 2)
            {
                std::stringstream msg;
                msg << "usage: " << argv[0] << " <configuration file path>";
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
            auto telemetryConfig = std::make_shared<Common::TelemetryConfigImpl::Config>(
                Common::TelemetryConfigImpl::Serialiser::deserialise(telemetryConfigJson));
            LOGDEBUG("Using configuration: " << Common::TelemetryConfigImpl::Serialiser::serialise(*telemetryConfig));

            std::unique_ptr<TelemetryProcessor> telemetryProcessor = initialiseTelemetryProcessor(telemetryConfig);

            telemetryProcessor->Run();

            return 0;
        }
        catch (const std::runtime_error& e)
        {
            LOGERROR("Error: " << e.what());
            return 1;
        }
    }
} // namespace Telemetry
