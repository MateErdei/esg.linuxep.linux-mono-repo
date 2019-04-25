/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include <Telemetry/LoggerImpl/Logger.h>

#include <sstream>
#include <string>

namespace Telemetry
{
    int main_entry(int argc, char* argv[], const std::shared_ptr<IHttpSender>& httpSender)
    {
        try
        {
            if (argc == 1 || argc > 4)
            {
                throw std::runtime_error(
                    "Telemetry executable expects the following arguments: verb [server] [cert path]");
            }

            std::vector<std::string> additionalHeaders;
            std::string verb = argv[1];
            if (argc >= 3)
            {
                std::string server = argv[2];
                httpSender->setServer(server);
            }

            std::string certPath = CERT_PATH;

            if (argc == 4)
            {
                certPath = argv[3];
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
            std::stringstream errorMsg;
            errorMsg << "Caught exception: " << e.what();
            LOGERROR(errorMsg.str());
            return 1;
        }

        return 0;
    }

} // namespace Telemetry
