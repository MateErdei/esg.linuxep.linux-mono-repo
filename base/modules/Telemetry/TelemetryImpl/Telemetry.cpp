/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include <Common/Logging/FileLoggingSetup.h>
#include "curl.h"

#include <sstream>

namespace Telemetry
{
    void http_request(
            const std::string& server,
            const int& port,
            const std::string& verb
            //const std::vector<std::string>& additionalHeaders,
            //const std::string& resourceRoot,
            //const int& interval
            )
    {
        std::stringstream uri;
        uri << "http://" << server << ":" << port; // TODO: This will be https, will need to change this

        std::stringstream msg;
        msg << "Creating HTTP " << verb << " Request to " << uri.str();
        LOGINFO(msg.str());

        CURL *curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if(curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, uri.str().c_str());

            /* Perform the request, res will get the return code */


            /* TODO: Add code to execute a PUT request
            if(verb.compare("PUT") == 0)
            {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=mel&project=telemetryrequest");
            }
            */

            res = curl_easy_perform(curl);
            /* Check for errors */
            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));

            /* always cleanup */
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
    }

    int main_entry(int argc, char* argv[])
    {
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("telemetry");

        std::stringstream msg;
        msg << "Running telemetry executable with arguments: ";
        for (int i = 0; i < argc; ++i)
        {
            msg << argv[i] << " ";
        }
        LOGINFO(msg.str());

        http_request(argv[1], std::stoi(argv[2]), argv[3]); // [LINUXEP-6075] This will be done via a configuration file

        return 0;
    }

} // namespace Telemetry
