/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry/HttpSender.h"
#include "Telemetry/Logger.h"

#include <Common/Logging/FileLoggingSetup.h>

namespace Telemetry
{
    int main_entry(int argc, char* argv[])
    {
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("telemetry");

        if (argc != 4) // argv[0] is the binary path
        {
            LOGERROR("Telemetry executable expects 3 arguments [server, port, verb]");
            return 1;
        }

        std::string server = argv[1];
        int port = std::stoi(argv[2]);
        std::string verb = argv[3];
        std::vector<std::string> additionalHeaders;
        additionalHeaders.emplace_back("x-amz-acl:bucket-owner-full-control"); // [LINUXEP-6075] This will be read in from a configuration file

        std::stringstream msg;
        msg << "Running telemetry executable with arguments: Server: " << server << ", Port: " << port << ", Verb: " << verb;
        LOGINFO(msg.str());

        HttpSender httpSender(server, port);

        if(verb == "POST") {
            std::string jsonStruct = "{ telemetryKey : telemetryValue }"; // [LINUXEP-6631] This will be specified later on
            httpSender.post_request(additionalHeaders, jsonStruct); // [LINUXEP-6075] This will be done via a configuration file
        }
        else
        {
            httpSender.get_request(additionalHeaders); // [LINUXEP-6075] This will be done via a configuration file
        }

        return 0;
    }

} // namespace Telemetry
