// Copyright 2023 Sophos Limited. All rights reserved.

#include "scan_messages/MetadataRescan.h"

#include <capnp/serialize.h>
#include <gtest/gtest.h>

using namespace scan_messages;

class TestMetadataRescan : public ::testing::Test
{
public:
    scan_messages::MetadataRescan metadataRescan_{ .filePath = "filePath",
                                                   .sha256 = "sha256",
                                                   .threat = {
                                                       .type = "threatType",
                                                       .name = "threatName",
                                                       .sha256 = "threatSha256",
                                                   } };
};

TEST_F(TestMetadataRescan, SerialiseThenDeserialiseReturnsObjectSameAsBeforeSerialisation)
{
    std::string dataAsString = metadataRescan_.Serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    const auto deserialised = MetadataRescan::Deserialise(view);

    EXPECT_EQ(metadataRescan_, deserialised);
}