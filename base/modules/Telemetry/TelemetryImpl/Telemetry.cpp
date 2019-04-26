/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <sstream>
#include <string>

namespace Telemetry
{
    int main(int argc, char* argv[], const std::shared_ptr<IHttpSender>& httpSender)
    {
        try
        {
            if (argc == 1 || argc > 4)
            {
                throw std::runtime_error(
                    "Telemetry executable expects the following arguments: request_type [server] [cert_path]");
            }

            std::vector<std::string> additionalHeaders;
            std::string verb = argv[1];
            if (argc >= 3)
            {
                std::string server = argv[2];
                httpSender->setServer(server);
            }

            std::string certPath = "/opt/sophos-spl/base/etc/sophosspl/telemetry_cert.pem";

            if (argc == 4)
            {
                certPath = argv[3];
            }

            if (!Common::FileSystem::fileSystem()->isFile(certPath))
            {
                throw std::runtime_error("Certificate is not a valid file");
            }

            additionalHeaders.emplace_back(
                "x-amz-acl:bucket-owner-full-control"); // [LINUXEP-6075] This will be read in from a configuration file

            if (verb == "POST")
            {
                std::string data =
                    "{ telemetryKey : telemetryValue }"; // [LINUXEP-6631] This will be specified later on
                httpSender->postRequest(
                    additionalHeaders, data, certPath); // [LINUXEP-6075] This will be done via a configuration file
            }
            else if (verb == "PUT")
            {
                std::string data =
                    "{ telemetryKey : telemetryValue }"; // [LINUXEP-6631] This will be specified later on
                httpSender->putRequest(
                    additionalHeaders, data, certPath); // [LINUXEP-6075] This will be done via a configuration file
            }
            else if (verb == "GET")
            {
                httpSender->getRequest(
                    additionalHeaders, certPath); // [LINUXEP-6075] This will be done via a configuration file
            }
            else
            {
                std::stringstream errorMsg;
                errorMsg << "Unexpected HTTPS request type: " << verb;
                throw std::runtime_error(errorMsg.str());
            }
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
        std::shared_ptr<IHttpSender> httpSender = std::make_shared<HttpSender>("https://t1.sophosupd.com/", curlWrapper);

        return main(argc, argv, httpSender);
    }

} // namespace Telemetry
