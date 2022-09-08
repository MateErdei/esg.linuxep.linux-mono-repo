/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatDetected.capnp.h"

#include <scan_messages/ThreatDetected.h>

#include <gtest/gtest.h>
#include <capnp/message.h>
#include <capnp/serialize.h>

#include <ctime>

using namespace scan_messages;

class TestThreatDetectedMessages : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_threatDetected.setUserID(m_userID);
        m_threatDetected.setDetectionTime(m_testTimeStamp);
        m_threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS_OPEN);
        m_threatDetected.setThreatName(m_threatName);
        m_threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
        m_threatDetected.setFilePath(m_filePath);
        m_threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);
        m_threatDetected.setSha256(m_sha256);
    }

    std::string m_userID = "TestUser";
    std::string m_threatName = "EICAR";
    std::string m_filePath = "/this/is/a/path/to/an/eicar";
    std::string m_sha256 = "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def";
    std::time_t m_testTimeStamp = std::time(nullptr);

    scan_messages::ThreatDetected m_threatDetected;
};


TEST_F(TestThreatDetectedMessages, CreateThreatDetected) //NOLINT
{
    std::string dataAsString = m_threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    EXPECT_EQ(deSerialisedData.getUserID(), m_userID);
    EXPECT_EQ(deSerialisedData.getDetectionTime(), m_testTimeStamp);
    EXPECT_EQ(deSerialisedData.getThreatType(), E_VIRUS_THREAT_TYPE);
    EXPECT_EQ(deSerialisedData.getThreatName(), m_threatName);
    EXPECT_EQ(deSerialisedData.getScanType(), E_SCAN_TYPE_ON_ACCESS_OPEN);
    EXPECT_EQ(deSerialisedData.getNotificationStatus(), E_NOTIFICATION_STATUS_CLEANED_UP);
    EXPECT_EQ(deSerialisedData.getFilePath(), m_filePath);
    EXPECT_EQ(deSerialisedData.getActionCode(), E_SMT_THREAT_ACTION_SHRED);
    EXPECT_EQ(deSerialisedData.getSha256(), m_sha256);
}

TEST_F(TestThreatDetectedMessages, CreateThreatDetected_emptyThreatName) //NOLINT
{
    m_threatDetected.setThreatName("");
    std::string dataAsString = m_threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
        messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    EXPECT_EQ(deSerialisedData.getUserID(), m_userID);
    EXPECT_EQ(deSerialisedData.getDetectionTime(), m_testTimeStamp);
    EXPECT_EQ(deSerialisedData.getThreatType(), E_VIRUS_THREAT_TYPE);
    EXPECT_EQ(deSerialisedData.getThreatName(), "");
    EXPECT_EQ(deSerialisedData.getScanType(), E_SCAN_TYPE_ON_ACCESS_OPEN);
    EXPECT_EQ(deSerialisedData.getNotificationStatus(), E_NOTIFICATION_STATUS_CLEANED_UP);
    EXPECT_EQ(deSerialisedData.getFilePath(), m_filePath);
    EXPECT_EQ(deSerialisedData.getActionCode(), E_SMT_THREAT_ACTION_SHRED);
}
