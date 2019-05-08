/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include "ISystemTelemetryCollector.h"
#include "SystemTelemetryCollectorImpl.h"
#include "SystemTelemetryReporter.h"
#include "TelemetryProcessor.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/HttpSenderImpl/RequestConfig.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <sstream>
#include <string>

namespace Telemetry
{
    static const int g_maxArgs = 5;

    int main(
        int argc,
        char** argv,
        const std::shared_ptr<Common::HttpSender::IHttpSender>& httpSender,
        TelemetryProcessor& telemetryProcessor)
    {
        try
        {
            // TODO: LINUXEP-7991 argument parsing is temporary as these parameters will be read in from a configuration
            // file
            if (argc == 1 || argc > g_maxArgs)
            {
                throw std::runtime_error(
                    "Telemetry executable expects the following arguments: request_type [server] [cert_path] [resource_root]");
            }

            std::vector<std::string> additionalHeaders;
            additionalHeaders.emplace_back(
                "x-amz-acl:bucket-owner-full-control"); // TODO: LINUXEP-7991 get from a configuration file

            std::shared_ptr<Common::HttpSender::RequestConfig> requestConfig = std::make_shared<Common::HttpSender::RequestConfig>(
                argv[1], additionalHeaders
            );

            if (argc >= 3)
            {
                requestConfig->setServer(argv[2]); // TODO: LINUXEP-7991 get from a configuration file
            }

            if (argc >= 4)
            {
                requestConfig->setCertPath(argv[3]); // TODO: LINUXEP-7991 get from a configuration file
            }

            if (argc == g_maxArgs)
            {
                requestConfig->setResourceRoot(argv[4]); // TODO: LINUXEP-7991 get from a configuration file
            }

            if (!Common::FileSystem::fileSystem()->isFile(requestConfig->getCertPath()))
            {
                throw std::runtime_error("Certificate is not a valid file");
            }

            telemetryProcessor.gatherTelemetry();
            telemetryProcessor.saveTelemetryToDisk(
                "telemetry.json"); // TODO: LINUXEP-7991 get path from a configuration file
            std::string telemetry = telemetryProcessor.getSerialisedTelemetry();
            requestConfig->setData(telemetry);

            httpSender->doHttpsRequest(requestConfig);
        }
        catch (const std::exception& e) // TODO: shouldn't catch std::exception - just let this abort the process?
        {
            LOGERROR("Caught exception: " << e.what());
            return 1;
        }

        return 0;
    }

    int main_entry(int argc, char* argv[])
    {
        Common::Logging::FileLoggingSetup loggerSetup("telemetry", true);

        std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper = std::make_shared<Common::HttpSender::CurlWrapper>();
        std::shared_ptr<Common::HttpSender::IHttpSender> httpSender = std::make_shared<Common::HttpSender::HttpSender>(curlWrapper);

        std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders;

        SystemTelemetryCollectorImpl systemTelemetryCollector(
            GL_systemTelemetryObjectsConfig, GL_systemTelemetryArraysConfig);
        std::shared_ptr<ITelemetryProvider> systemTelemetryReporter =
            std::make_shared<SystemTelemetryReporter>(systemTelemetryCollector);
        telemetryProviders.emplace_back(systemTelemetryReporter);

        TelemetryProcessor telemetryProcessor(telemetryProviders);

        return main(argc, argv, httpSender, telemetryProcessor);
    }

} // namespace Telemetry
