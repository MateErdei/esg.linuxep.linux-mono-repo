// Copyright 2019-2022, Sophos Limited. All rights reserved.

#include "ThreatDetected.capnp.h"

#include "scan_messages/ThreatDetected.h"

#include <capnp/serialize.h>
#include <gtest/gtest.h>

#include <ctime>

using namespace scan_messages;
using namespace common::CentralEnums;

class TestThreatDetectedMessages : public ::testing::Test
{
public:
    ThreatDetected createThreatDetected(const std::string& threatName, const std::string& threatId)
    {
        return ThreatDetected { m_userID,
                                m_testTimeStamp,
                                ThreatType::virus,
                                threatName,
                                E_SCAN_TYPE_ON_ACCESS_OPEN,
                                E_NOTIFICATION_STATUS_CLEANED_UP,
                                m_filePath,
                                E_SMT_THREAT_ACTION_SHRED,
                                m_sha256,
                                threatId,
                                true,
                                ReportSource::ml,
                                datatypes::AutoFd() };
    }

    std::string m_userID = "TestUser";
    std::string m_threatName = "EICAR";
    std::string m_filePath = "/this/is/a/path/to/an/eicar";
    std::string m_sha256 = "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def";
    std::time_t m_testTimeStamp = std::time(nullptr);
    std::string m_threatId = "01234567-89ab-cdef-0123-456789abcdef";
};

TEST_F(TestThreatDetectedMessages, CreateThreatDetected)
{
    const ThreatDetected threatDetected = createThreatDetected(m_threatName, m_threatId);
    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    ThreatDetected deserialisedThreatDetected(deSerialisedData);

    EXPECT_EQ(deserialisedThreatDetected, threatDetected);
    EXPECT_EQ(-1, threatDetected.pid);
    EXPECT_EQ("", threatDetected.executablePath);
    EXPECT_EQ(-1, deserialisedThreatDetected.pid);
    EXPECT_EQ("", deserialisedThreatDetected.executablePath);
}
TEST_F(TestThreatDetectedMessages, CreateThreatDetectedSetOnAcesssValues)
{
    ThreatDetected threatDetected = createThreatDetected(m_threatName, m_threatId);
    threatDetected.pid = 100;
    threatDetected.executablePath = "/random/path";
    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    ThreatDetected deserialisedThreatDetected(deSerialisedData);

    EXPECT_EQ(deserialisedThreatDetected, threatDetected);
    EXPECT_EQ(100, deserialisedThreatDetected.pid);
    EXPECT_EQ("/random/path", deserialisedThreatDetected.executablePath);
}

TEST_F(TestThreatDetectedMessages, CreateThreatDetected_emptyThreatName)
{
    const ThreatDetected threatDetected = createThreatDetected("", m_threatId);
    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    ThreatDetected deserialisedThreatDetected(deSerialisedData);
    EXPECT_EQ(deserialisedThreatDetected, threatDetected);
}

TEST_F(TestThreatDetectedMessages, constructorThrowsOnInvalidThreatId)
{
    EXPECT_THROW(createThreatDetected(m_threatName, "invalid threat id"), std::runtime_error);
}

TEST_F(TestThreatDetectedMessages, serialiseThrowsOnInvalidThreatId)
{
    ThreatDetected threatDetected = createThreatDetected(m_threatName, m_threatId);
    threatDetected.threatId = "invalid threat id";
    EXPECT_THROW(std::ignore = threatDetected.serialise(), std::runtime_error);
}

TEST_F(TestThreatDetectedMessages, constructorThrowsWhenSerialisingFromInvalidThreatId)
{
    const ThreatDetected threatDetected = createThreatDetected(m_threatName, m_threatId);
    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    // Change the threatId before serialising back
    capnp::FlatArrayMessageReader messageReader(view);
    ::capnp::MallocMessageBuilder messageBuilder;
    messageBuilder.setRoot(messageReader.getRoot<Sophos::ssplav::ThreatDetected>());
    Sophos::ssplav::ThreatDetected::Builder builder = messageBuilder.getRoot<Sophos::ssplav::ThreatDetected>();
    builder.setThreatId("invalid threat id");
    Sophos::ssplav::ThreatDetected::Reader reader = builder.asReader();

    EXPECT_THROW(ThreatDetected { reader }, std::runtime_error);
}