/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/Print.h"
#include "thirdparty/nlohmann-json/json.hpp"

#include <gtest/gtest.h>

using json = nlohmann::json;

static std::string susiResponseStr =
        "{\n"
        "    \"results\":\n"
        "    [\n"
        "        {\n"
        "            \"detections\": [\n"
        "               {\n"
        "                   \"threatName\": \"MAL/malware-A\",\n"
        "                   \"threatType\": \"malware/trojan/PUA/...\",\n"
        "                   \"threatPath\": \"...\",\n"
        "                   \"longName\": \"...\",\n"
        "                   \"identityType\": 0,\n"
        "                   \"identitySubtype\": 0\n"
        "               }\n"
        "            ],\n"
        "            \"Local\": {\n"
        "                \"LookupType\": 1,\n"
        "                \"Score\": 20,\n"
        "                \"SignerStrength\": 30\n"
        "            },\n"
        "            \"Global\": {\n"
        "                \"LookupType\": 1,\n"
        "                \"Score\": 40\n"
        "            },\n"
        "            \"ML\": {\n"
        "                \"ml-pe\": 50\n"
        "            },\n"
        "            \"properties\": {\n"
        "                \"GENES/SUPPRESSML\": 1\n"
        "            },\n"
        "            \"telemetry\": [\n"
        "                {\n"
        "                    \"identityName\": \"xxx\",\n"
        "                    \"dataHex\": \"6865646765686f67\"\n"
        "                }\n"
        "            ],\n"
        "            \"mlscores\": [\n"
        "                {\n"
        "                    \"score\": 12,\n"
        "                    \"secScore\": 34,\n"
        "                    \"featuresHex\": \"6865646765686f67\",\n"
        "                    \"mlDataVersion\": 56\n"
        "                }\n"
        "            ]\n"
        "        }\n"
        "    ]\n"
        "}";

TEST(TestSusiResponseParsing, test_response_parsing) //NOLINT
{
    json response = json::parse(susiResponseStr);
    int count = 0;

    for( auto result : response["results"] )
    {
        for (auto detection : result["detections"])
        {
            PRINT("Detected " << detection["threatName"] << " in " << detection["threatPath"]);
            ASSERT_EQ(detection["threatName"], "MAL/malware-A");
            ASSERT_EQ(detection["threatPath"], "...");
            ASSERT_EQ(++count, 1);
        }
    }


}

static std::string susiRequestStr =
        "{\n"
        "    \"apiVersion\": \"1.0\",\n"
        "    \"path\": \"/usr/bin/malware.zip\",\n"
        "    \"scan-type\": \"jit\",\n"
        "    \"time\": \"2019-03-04 02:55Z\",\n"
        "    \"sha256\": \"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\"\n"
        "}\n";

static std::string flatRequestStr =
        "{"
        "\"apiVersion\":\"1.0\","
        "\"path\":\"/usr/bin/malware.zip\","
        "\"scan-type\":\"jit\","
        "\"sha256\":\"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\","
        "\"time\":\"2019-03-04 02:55Z\""
        "}";

TEST(TestSusiResponseParsing, test_request_serialization) //NOLINT
{
    json request;

    request["apiVersion"] = "1.0";
    request["path"] = "/usr/bin/malware.zip";
    request["scan-type"] = "jit";
    request["time"] = "2019-03-04 02:55Z";
    request["sha256"] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    std::string requestStr = request.dump();
    PRINT(requestStr);

    ASSERT_EQ(requestStr, flatRequestStr);

    json susiRequest = json::parse(susiRequestStr);
    ASSERT_EQ(request, susiRequest);
    ASSERT_EQ(requestStr, susiRequest.dump());
}