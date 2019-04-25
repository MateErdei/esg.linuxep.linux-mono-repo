/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

#include <Telemetry/LoggerImpl/Logger.h>
#include <openssl/hmac.h>

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace Telemetry
{
    unsigned char* hashString(const std::string& data, const std::string& key)
    {

        // Be careful of the length of string with the choosen hash engine. SHA1 needed 20 characters.
        // Change the length accordingly with your choosen hash engine.
        unsigned char* result;
        unsigned int len = 20;

        result = (unsigned char*)malloc(sizeof(char) * len);

        HMAC_CTX *h = HMAC_CTX_new();

        // Using sha1 hash engine here.
        // You may use other hash engines. e.g EVP_md5(), EVP_sha224, EVP_sha512, etc
        HMAC_Init_ex(h, key.c_str(), key.length(), EVP_sha1(), nullptr);
        HMAC_Update(h, (unsigned char*)data.c_str(), data.length());
        HMAC_Final(h, result, &len);
        HMAC_CTX_free(h);

        return result;
    }

    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for(i = 0; (i <4) ; i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i)
        {
            for(j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

            while((i++ < 3))
                ret += '=';

        }

        return ret;

    }

    std::string getDateTime()
    {
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        // TODO e.g. Wed, 24 Apr 2019 16:23:26 +0100
        strftime(buffer,sizeof(buffer),"%a, %d %b %Y %H:%M:%S %z",timeinfo);
        std::string str(buffer);

        return str;
    }

    int main_entry(int argc, char* argv[], std::shared_ptr<IHttpSender> httpSender)
    {
        try
        {
            if (argc == 1 || argc == 3 || argc > 5)
            {
                throw std::runtime_error("Telemetry executable expects either 1 (verb), 3 (verb, server, port) or 4 (verb, server, port, cert path) arguments");
            }

            std::string bucket = "sspl-testbucket"; // TODO: Set to the production S3 bucket
            std::vector<std::string> additionalHeaders;
            std::string verb = argv[1];
            if (argc >= 4)
            {
                std::string server = argv[2];
                int port = std::stoi(argv[3]);

                std::stringstream bucketURL;
                bucketURL << bucket << "." << server;

                httpSender->setServer(bucketURL.str());
                httpSender->setPort(port);

                // Host Header
                std::stringstream hostHeader;
                hostHeader << "Host: " << bucketURL.str();
                additionalHeaders.emplace_back(hostHeader.str());
            }

            std::string certPath = CERT_PATH;

            if (argc == 5)
            {
                certPath = argv[4];
            }

            // Date Header
            std::string dateFormatted = getDateTime();

            std::stringstream dateHeader;
            dateHeader << "Date: " << dateFormatted;
            additionalHeaders.emplace_back(dateHeader.str());

            // Authorisation Header
            std::stringstream stringToSign;
            stringToSign << "PUT\n\napplication/octet-stream\n" << dateFormatted << "\n/" << bucket << "/test.json";
            LOGINFO(stringToSign.str());

            std::string secretKey = "09/KeoBM/fhfj9AQOwaRpSXAwOATTcEe3PKL/V7v";
            std::stringstream authHeader;
            unsigned char* hexSignature = hashString(stringToSign.str(), secretKey);
            std::string base64signature(base64_encode(hexSignature, 20));
            authHeader << "Authorization: AWS AKIAIF23TRE42IG5IH4Q:" << base64signature;
            additionalHeaders.emplace_back(authHeader.str());

            additionalHeaders.emplace_back(
                    "x-amz-acl:bucket-owner-full-control"); // [LINUXEP-6075] This will be read in from a configuration file

            if (verb == "POST")
            {
                additionalHeaders.emplace_back("Content-Type: application/octet-stream");
                std::string jsonStruct = "{ telemetryKey : telemetryValue }"; // [LINUXEP-6631] This will be specified later on
                httpSender->postRequest(additionalHeaders, jsonStruct, certPath); // [LINUXEP-6075] This will be done via a configuration file
            }
            else if (verb == "PUT")
            {
                additionalHeaders.emplace_back("Content-Type: application/octet-stream");
                std::string jsonStruct = "{ telemetryKey : telemetryValue }"; // [LINUXEP-6631] This will be specified later on
                httpSender->putRequest(additionalHeaders, jsonStruct, certPath); // [LINUXEP-6075] This will be done via a configuration file
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
