/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include <thirdparty/nlohmann-json/json.hpp>
#include <datatypes/Print.h>

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
    nlohmann::json jsonObj = nlohmann::json::parse(susiResponseStr);
    int count = 0;

    for( auto result : jsonObj["results"] )
    {
        for (auto detection : result["detections"])
        {
            PRINT("Detected " << detection["threatName"] << " in " << detection["threatPath"]);
            ASSERT_EQ(detection["threatName"], "MAL/malware-A");
            ASSERT_EQ(detection["threatPath"], "...");
            count++;
        }
    }

    ASSERT_EQ(count, 1);
}