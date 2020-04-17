/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanResponse.capnp.h"

#include <scan_messages/ScanResponse.h>

#include <gtest/gtest.h>
#include <capnp/message.h>
#include <capnp/serialize.h>

#include <ctime>

using namespace scan_messages;

class TestScanResponseMessages : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_scanResponse.setClean(m_clean);
        m_scanResponse.setThreatName(m_threatName);
        m_scanResponse.setFullScanResult(m_fullScanResult);
    }

    bool m_clean = true;
    std::string m_threatName = "EICAR-AV-TEST";
    std::string m_fullScanResult = "{\n"
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

    scan_messages::ScanResponse m_scanResponse;
};


TEST_F(TestScanResponseMessages, CreateScanResponse) //NOLINT
{
std::string dataAsString = m_scanResponse.serialise();

const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

capnp::FlatArrayMessageReader messageInput(view);
Sophos::ssplav::FileScanResponse::Reader deSerialisedData =
        messageInput.getRoot<Sophos::ssplav::FileScanResponse>();

EXPECT_EQ(deSerialisedData.getClean(), m_clean);
EXPECT_EQ(deSerialisedData.getThreatName(), m_threatName);
EXPECT_EQ(deSerialisedData.getFullScanResult(), m_fullScanResult);
}