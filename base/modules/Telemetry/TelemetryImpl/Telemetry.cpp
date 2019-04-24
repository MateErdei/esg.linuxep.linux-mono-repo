/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include <Telemetry/LoggerImpl/Logger.h>

#include <sstream>

namespace Telemetry
{
    int main_entry(int argc, char* argv[], std::shared_ptr<IHttpSender> httpSender)
    {
        try
        {
            if (argc == 1 || argc == 3 || argc > 5)
            {
                throw std::runtime_error("Telemetry executable expects either 1 (verb), 3 (verb, server, port) or 4 (verb, server, port, cert path) arguments");
            }

            std::string verb = argv[1];
            std::vector<std::string> additionalHeaders;
            additionalHeaders.emplace_back(
                    "x-amz-acl:bucket-owner-full-control"); // [LINUXEP-6075] This will be read in from a configuration file

            if (argc >= 4)
            {
                std::string server = argv[2];
                int port = std::stoi(argv[3]);

                httpSender->setServer(server);
                httpSender->setPort(port);
            }

            std::string certPath = CERT_PATH;

            if (argc == 5)
            {
                certPath = argv[4];
            }

            if (verb == "POST")
            {
                std::string jsonStruct = "{ telemetryKey : telemetryValue }"; // [LINUXEP-6631] This will be specified later on
                httpSender->postRequest(additionalHeaders, jsonStruct, certPath); // [LINUXEP-6075] This will be done via a configuration file
            }
            else if (verb == "GET")
            {
                httpSender->getRequest(additionalHeaders, certPath); // [LINUXEP-6075] This will be done via a configuration file
            }
            else
            {
                std::stringstream errorMsg;
                errorMsg << "Unexpected HTTPS request type: " << verb;
                throw std::runtime_error(errorMsg.str());
            }
        }
        catch(const std::exception& e)
        {
            std::stringstream errorMsg;
            errorMsg << "Caught exception: " << e.what();
            LOGERROR(errorMsg.str());
            return 1;
        }

        return 0;
    }

} // namespace Telemetry
