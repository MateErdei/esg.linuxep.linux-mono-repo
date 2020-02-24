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
        threatDetected.setUserID(userID);
        threatDetected.setDetectionTime(testTimeStamp);
        threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
        threatDetected.setThreatName(threatName);
        threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
        threatDetected.setFilePath(filePath);
        threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);
    }

    std::string userID = "TestUser";
    std::string threatName = "EICAR";
    std::string filePath = "/this/is/a/path/to/an/eicar";

    scan_messages::ThreatDetected threatDetected;
    std::time_t testTimeStamp = std::time(nullptr);
};


TEST_F(TestThreatDetectedMessages, CreateThreatDetected) //NOLINT
{
    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    EXPECT_EQ(deSerialisedData.getUserID(), userID);
    EXPECT_EQ(deSerialisedData.getDetectionTime(), testTimeStamp);
    EXPECT_EQ(deSerialisedData.getThreatType(), E_VIRUS_THREAT_TYPE);
    EXPECT_EQ(deSerialisedData.getThreatName(), threatName);
    EXPECT_EQ(deSerialisedData.getScanType(), E_SCAN_TYPE_ON_ACCESS);
    EXPECT_EQ(deSerialisedData.getNotificationStatus(), E_NOTIFICATION_STATUS_CLEANED_UP);
    EXPECT_EQ(deSerialisedData.getFilePath(), filePath);
    EXPECT_EQ(deSerialisedData.getActionCode(), E_SMT_THREAT_ACTION_SHRED);
}