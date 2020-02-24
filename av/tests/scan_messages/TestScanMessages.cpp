/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <scan_messages/ThreatDetected.h>
#include <scan_messages/ScanRequest.h>

#include "ScanRequest.capnp.h"
#include "ThreatDetected.capnp.h"


#include <ctime>

TEST(TestScanMessages, CreateScanRequestSerialisation) // NOLINT
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname("/etc/fstab");
    requestBuilder.setScanInsideArchives(true);

    // Convert to byte string
    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());


    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::FileScanRequest::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::FileScanRequest>();

    EXPECT_EQ(requestReader.getPathname(), "/etc/fstab");
    EXPECT_TRUE(requestReader.getScanInsideArchives());
}

TEST(TestScanMessages, CreateScanRequestObject) // NOLINT
{

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname("/etc/fstab");
    requestBuilder.setScanInsideArchives(false);

    Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;

    auto scanRequest = std::make_unique<scan_messages::ScanRequest>(-1, requestReader);

    EXPECT_EQ(scanRequest->fd(), -1);
    EXPECT_EQ(scanRequest->path(), "/etc/fstab");
    EXPECT_FALSE(scanRequest->scanInsideArchives());
}

TEST(TestScanMessages, ReuseScanRequestObject) // NOLINT
{
    auto scanRequest = std::make_unique<scan_messages::ScanRequest>();

    {
        ::capnp::MallocMessageBuilder message;
        Sophos::ssplav::FileScanRequest::Builder requestBuilder =
                message.initRoot<Sophos::ssplav::FileScanRequest>();
        requestBuilder.setPathname("/etc/fstab");
        requestBuilder.setScanInsideArchives(true);
        Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;
        scanRequest->resetRequest(-1, requestReader);
    }

    EXPECT_EQ(scanRequest->fd(), -1);
    EXPECT_EQ(scanRequest->path(), "/etc/fstab");
    EXPECT_TRUE(scanRequest->scanInsideArchives());

    {
        ::capnp::MallocMessageBuilder message;
        Sophos::ssplav::FileScanRequest::Builder requestBuilder =
                message.initRoot<Sophos::ssplav::FileScanRequest>();
        requestBuilder.setPathname("/etc/shadow");
        requestBuilder.setScanInsideArchives(false);
        Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;
        scanRequest->resetRequest(-1, requestReader);
    }

    EXPECT_EQ(scanRequest->fd(), -1);
    EXPECT_EQ(scanRequest->path(), "/etc/shadow");
    EXPECT_FALSE(scanRequest->scanInsideArchives());
}

TEST(TestScanMessages, CreateThreatDetected) //NOLINT
{
    scan_messages::ThreatDetected threatDetected;
    std::time_t testTimeStamp = std::time(nullptr);

    threatDetected.setUserID("TestUser");
    threatDetected.setDetectionTime(std::to_string(testTimeStamp));
    threatDetected.setScanType("201");
    threatDetected.setNotificationStatus("300");
    threatDetected.setThreatID("EICAR-AV-Test");
    threatDetected.setFileName("unit-test-eicar");
    threatDetected.setFilePath("/this/is/a/path");
    threatDetected.setActionCode("113");

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();
    EXPECT_EQ(deSerialisedData.getUserID(), "TestUser");
    EXPECT_EQ(deSerialisedData.getDetectionTime(), std::to_string(testTimeStamp));
    EXPECT_EQ(deSerialisedData.getThreatType(), "1");
    EXPECT_EQ(deSerialisedData.getScanType(), "201");
    EXPECT_EQ(deSerialisedData.getNotificationStatus(), "300");
    EXPECT_EQ(deSerialisedData.getThreatID(), "EICAR-AV-Test");
    EXPECT_EQ(deSerialisedData.getIdSource(), "NameFilenameFilepathCIMD5");
    EXPECT_EQ(deSerialisedData.getFileName(), "unit-test-eicar");
    EXPECT_EQ(deSerialisedData.getFilePath(), "/this/is/a/path");
    EXPECT_EQ(deSerialisedData.getActionCode(), "113");
}

TEST(TestScanMessages, CreateThreatDetectedWithCustomIdAndThreatType) //NOLINT
{
    scan_messages::ThreatDetected threatDetected;
    std::time_t testTimeStamp = std::time(nullptr);

    threatDetected.setUserID("TestUser");
    threatDetected.setDetectionTime(std::to_string(testTimeStamp));
    threatDetected.setThreatType("5");
    threatDetected.setScanType("201");
    threatDetected.setNotificationStatus("300");
    threatDetected.setThreatID("EICAR-AV-Test");
    threatDetected.setIdSource("CustomIdSource");
    threatDetected.setFileName("unit-test-eicar");
    threatDetected.setFilePath("/this/is/a/path");
    threatDetected.setActionCode("113");

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    EXPECT_EQ(deSerialisedData.getUserID(), "TestUser");
    EXPECT_EQ(deSerialisedData.getDetectionTime(), std::to_string(testTimeStamp));
    EXPECT_EQ(deSerialisedData.getThreatType(), "5");
    EXPECT_EQ(deSerialisedData.getScanType(), "201");
    EXPECT_EQ(deSerialisedData.getNotificationStatus(), "300");
    EXPECT_EQ(deSerialisedData.getThreatID(), "EICAR-AV-Test");
    EXPECT_EQ(deSerialisedData.getIdSource(), "CustomIdSource");
    EXPECT_EQ(deSerialisedData.getFileName(), "unit-test-eicar");
    EXPECT_EQ(deSerialisedData.getFilePath(), "/this/is/a/path");
    EXPECT_EQ(deSerialisedData.getActionCode(), "113");
}