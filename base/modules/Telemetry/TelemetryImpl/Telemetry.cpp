/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include <Common/Logging/FileLoggingSetup.h>
#include "curl.h"

#include <sstream>

namespace Telemetry
{
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


        CURL *curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:4443");

            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl);
            /* Check for errors */
            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));

            /* always cleanup */
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        return 0;
    }

} // namespace Telemetry
