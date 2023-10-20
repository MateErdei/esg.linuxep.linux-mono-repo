// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "SampleThreatDetected.h"
#include "scan_messages/ThreatDetected.capnp.h"

#include "scan_messages/ThreatDetected.h"

#include <capnp/serialize.h>
#include <gtest/gtest.h>

using namespace scan_messages;
using namespace common::CentralEnums;

namespace
{
    class TestThreatDetectedMessages : public ::testing::Test
    {
    };
} // namespace

TEST_F(TestThreatDetectedMessages, CreateThreatDetected)
{
    const ThreatDetected threatDetected = createThreatDetected({});
    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    ThreatDetected deserialisedThreatDetected = ThreatDetected::deserialise(deSerialisedData);

    EXPECT_EQ(deserialisedThreatDetected, threatDetected);
}
TEST_F(TestThreatDetectedMessages, CreateThreatDetectedSetOnAcesssValues)
{
    ThreatDetected threatDetected = createOnAccessThreatDetected({});
    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    ThreatDetected deserialisedThreatDetected = ThreatDetected::deserialise(deSerialisedData);

    EXPECT_EQ(deserialisedThreatDetected, threatDetected);
}

TEST_F(TestThreatDetectedMessages, serialiseThrowsOnEmptyThreatName)
{
    ThreatDetected threatDetected = createThreatDetected({ .threatName = "" });
    EXPECT_ANY_THROW(std::ignore = threatDetected.serialise());
}

TEST_F(TestThreatDetectedMessages, serialiseThrowsOnEmptyPath)
{
    ThreatDetected threatDetected = createThreatDetected({ .filePath = "" });
    EXPECT_ANY_THROW(std::ignore = threatDetected.serialise());
}

TEST_F(TestThreatDetectedMessages, serialiseThrowsOnInvalidThreatId)
{
    ThreatDetected threatDetected = createThreatDetected({ .threatId = "invalid threat id" });
    EXPECT_ANY_THROW(std::ignore = threatDetected.serialise());
}

TEST_F(TestThreatDetectedMessages, serialiseThrowsOnInvalidCorrelationId)
{
    ThreatDetected threatDetected = createThreatDetected({ .correlationId = "invalid correlation id" });
    EXPECT_ANY_THROW(std::ignore = threatDetected.serialise());
}

TEST_F(TestThreatDetectedMessages, deserialiseThrowsOnEmptyThreatName)
{
    const ThreatDetected threatDetected = createThreatDetected({});
    std::string dataAsString = threatDetected.serialise();
    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));
    ::capnp::FlatArrayMessageReader messageReader(view);
    ::capnp::MallocMessageBuilder messageBuilder;
    messageBuilder.setRoot(messageReader.getRoot<Sophos::ssplav::ThreatDetected>());
    auto builder = messageBuilder.getRoot<Sophos::ssplav::ThreatDetected>();

    builder.setThreatName("");
    Sophos::ssplav::ThreatDetected::Reader reader = builder.asReader();

    EXPECT_ANY_THROW(std::ignore = ThreatDetected::deserialise(reader));
}

TEST_F(TestThreatDetectedMessages, deserialiseThrowsOnEmptyPath)
{
    const ThreatDetected threatDetected = createThreatDetected({});
    std::string dataAsString = threatDetected.serialise();
    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));
    ::capnp::FlatArrayMessageReader messageReader(view);
    ::capnp::MallocMessageBuilder messageBuilder;
    messageBuilder.setRoot(messageReader.getRoot<Sophos::ssplav::ThreatDetected>());
    auto builder = messageBuilder.getRoot<Sophos::ssplav::ThreatDetected>();

    builder.setFilePath("");
    Sophos::ssplav::ThreatDetected::Reader reader = builder.asReader();

    EXPECT_ANY_THROW(std::ignore = ThreatDetected::deserialise(reader));
}

TEST_F(TestThreatDetectedMessages, deserialiseThrowsOnEmptyThreatId)
{
    const ThreatDetected threatDetected = createThreatDetected({});
    std::string dataAsString = threatDetected.serialise();
    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));
    ::capnp::FlatArrayMessageReader messageReader(view);
    ::capnp::MallocMessageBuilder messageBuilder;
    messageBuilder.setRoot(messageReader.getRoot<Sophos::ssplav::ThreatDetected>());
    auto builder = messageBuilder.getRoot<Sophos::ssplav::ThreatDetected>();

    builder.setThreatId("invalid threat id");
    Sophos::ssplav::ThreatDetected::Reader reader = builder.asReader();

    EXPECT_ANY_THROW(std::ignore = ThreatDetected::deserialise(reader));
}

TEST_F(TestThreatDetectedMessages, deserialiseThrowsOnInvalidCorrelationId)
{
    const ThreatDetected threatDetected = createThreatDetected({});
    std::string dataAsString = threatDetected.serialise();
    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));
    ::capnp::FlatArrayMessageReader messageReader(view);
    ::capnp::MallocMessageBuilder messageBuilder;
    messageBuilder.setRoot(messageReader.getRoot<Sophos::ssplav::ThreatDetected>());
    auto builder = messageBuilder.getRoot<Sophos::ssplav::ThreatDetected>();

    builder.setCorrelationId("invalid correlation id");
    Sophos::ssplav::ThreatDetected::Reader reader = builder.asReader();

    EXPECT_ANY_THROW(std::ignore = ThreatDetected::deserialise(reader));
}