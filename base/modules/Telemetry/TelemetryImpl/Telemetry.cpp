/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/HttpSenderImpl/RequestConfig.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <sstream>
#include <string>

namespace Telemetry
{
    static const int g_maxArgs = 5;

    int main(int argc, char* argv[], Common::HttpSender::IHttpSender& httpSender)
    {
        try
        {
            // TODO: [LINUXEP-6075] All argument parsing is temporary as these parameters will be read in from a configuration file
            if (argc == 1 || argc > g_maxArgs)
            {
                throw std::runtime_error(
                    "Telemetry executable expects the following arguments: request_type [server] [cert_path] [resource_root]");
            }

            std::vector<std::string> additionalHeaders;
            additionalHeaders.emplace_back(
                "x-amz-acl:bucket-owner-full-control"); // TODO: [LINUXEP-6075] This will be read in from a configuration file

            Common::HttpSenderImpl::RequestConfig requestConfig(argv[1], additionalHeaders);

            if (argc >= 3)
            {
                requestConfig.setServer(argv[2]); // TODO: [LINUXEP-6075] This will be read in from a configuration file
            }

            if (argc >= 4)
            {
                requestConfig.setCertPath(argv[3]); // TODO: [LINUXEP-6075] This will be read in from a configuration file
            }

            if (argc == g_maxArgs)
            {
                requestConfig.setResourceRoot(argv[4]); // TODO: [LINUXEP-6075] This will be read in from a configuration file
            }

            if (!Common::FileSystem::fileSystem()->isFile(requestConfig.getCertPath()))
            {
                throw std::runtime_error("Certificate is not a valid file");
            }

            requestConfig.setData("{ telemetryKey : telemetryValue }"); // TODO: [LINUXEP-6075] This will be read in from a configuration file
            httpSender.doHttpsRequest(requestConfig);
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
        try
        {
            // Configure logging
            Common::Logging::FileLoggingSetup loggerSetup("telemetry", true);

            std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper =
                std::make_shared<Common::HttpSenderImpl::CurlWrapper>();
            Common::HttpSenderImpl::HttpSender httpSender(curlWrapper);

            return main(argc, argv, httpSender);
        }
        catch (const std::runtime_error& e)
        {
            LOGERROR("Caught exception: " << e.what());
            return 1;
        }
    }

} // LCOV_EXCL_LINE // namespace Telemetry
