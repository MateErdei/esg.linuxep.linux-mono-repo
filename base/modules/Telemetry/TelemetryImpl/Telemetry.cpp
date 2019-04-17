/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include <Common/Logging/FileLoggingSetup.h>
#include "curl.h"

#include <fstream>
#include <sstream>

namespace Telemetry
{
    void http_request(
            const std::string& server,
            const int& port,
            const std::string& verb,
            const std::vector<std::string>& additionalHeaders=std::vector<std::string>(),
            const std::string& jsonStruct=""
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

            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "charsets: utf-8");
            for (auto const& header : additionalHeaders)
            {
                headers = curl_slist_append(headers, header.c_str());
            }

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            if(verb.compare("POST") == 0)
            {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStruct.c_str());
            }

            res = curl_easy_perform(curl);

            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
    }

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

        if(verb.compare("POST") == 0) {
            std::string jsonStruct = "{ telemetryKey : telemetryValue }"; // [LINUXEP-6631] This will be specified later on
            http_request(server, port, verb, additionalHeaders, jsonStruct); // [LINUXEP-6075] This will be done via a configuration file
        }
        else
        {
            http_request(server, port, verb, additionalHeaders); // [LINUXEP-6075] This will be done via a configuration file
        }

        return 0;
    }

} // namespace Telemetry
