/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include "TelemetryProcessor.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Telemetry/HttpSenderImpl/RequestConfig.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <sstream>
#include <string>

namespace Telemetry
{
    static const int g_maxArgs = 5;

    int main(int argc, char* argv[], const std::shared_ptr<IHttpSender>& httpSender)
    {
        try
        {
            if (argc == 1 || argc > g_maxArgs)
            {
                throw std::runtime_error("Telemetry executable expects the following arguments: request_type [server] "
                                         "[cert_path] [resource_root]");
            }

            std::vector<std::string> additionalHeaders;
            additionalHeaders.emplace_back(
                "x-amz-acl:bucket-owner-full-control"); // [LINUXEP-6075] This will be read in from a configuration file

            std::shared_ptr<RequestConfig> requestConfig = std::make_shared<RequestConfig>(argv[1], additionalHeaders);

            if (argc >= 3)
            {
                requestConfig->setServer(argv[2]);
            }

            if (argc >= 4)
            {
                requestConfig->setCertPath(argv[3]);
            }

            if (argc == g_maxArgs)
            {
                requestConfig->setResourceRoot(argv[4]);
            }

            if (!Common::FileSystem::fileSystem()->isFile(requestConfig->getCertPath()))
            {
                throw std::runtime_error("Certificate is not a valid file");
            }

            requestConfig->setData(
                "{ telemetryKey : telemetryValue }"); // [LINUXEP-6631] This will be specified later on
            httpSender->httpsRequest(requestConfig);  // [LINUXEP-6075] This will be done via a configuration file
        }
        catch (const std::exception& e)
        {
            LOGERROR("Caught exception: " << e.what());
            return 1;
        }

        return 0;
    }

    int main_entry(int argc, char* argv[])
    {
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("telemetry", true);

        std::shared_ptr<ICurlWrapper> curlWrapper = std::make_shared<CurlWrapper>();
        std::shared_ptr<IHttpSender> httpSender = std::make_shared<HttpSender>(curlWrapper);

        return main(argc, argv, httpSender);
    }

} // namespace Telemetry
