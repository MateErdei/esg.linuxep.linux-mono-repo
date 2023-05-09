// Copyright 2023 Sophos Limited. All rights reserved.

#include "scan_messages/MetadataRescan.h"

#include <gtest/gtest.h>

using namespace scan_messages;

class TestThreat : public ::testing::Test
{
public:
    Threat threat_{
        .type = "threatType",
        .name = "threatName",
        .sha256 = "threatSha256",
    };
};

TEST_F(TestThreat, SerialiseThenDeserialiseReturnsObjectSameAsBeforeSerialisation)
{
    nlohmann::json j(threat_);
    const auto dataAsString = j.dump();
    j = nlohmann::json::parse(dataAsString);
    const auto deserialised = j.get<Threat>();
    EXPECT_EQ(threat_, deserialised);
}