// Copyright 2019-2022 Sophos Limited. All rights reserved.

#include "ScanRequest.capnp.h"

#include <scan_messages/ScanRequest.h>

#include "datatypes/MockSysCalls.h"

#include <gtest/gtest.h>
#include <capnp/message.h>
#include <capnp/serialize.h>

TEST(TestScanMessages, CreateScanRequestSerialisation)
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname("/etc/fstab");
    requestBuilder.setScanInsideArchives(true);
    requestBuilder.setScanType(scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS);
    requestBuilder.setUserID("sophos-spl");
    requestBuilder.setPid(3);
    requestBuilder.setExecutablePath("/tmp/testpath");

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
    EXPECT_EQ(requestReader.getScanType(), scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS);
    EXPECT_EQ(requestReader.getUserID(), "sophos-spl");
    EXPECT_EQ(requestReader.getPid(), 3);
    EXPECT_EQ(requestReader.getExecutablePath(), "/tmp/testpath");
    EXPECT_TRUE(requestReader.getScanInsideArchives());
}

TEST(TestScanMessages, CreateScanRequestSerialisationOnDemandHasCorrectDefaultValues)
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname("/etc/fstab");
    requestBuilder.setScanInsideArchives(true);
    requestBuilder.setScanType(scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_DEMAND);
    requestBuilder.setUserID("sophos-spl");

    EXPECT_EQ(requestBuilder.getPid(), -1);
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
    EXPECT_EQ(requestReader.getScanType(), scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_DEMAND);
    EXPECT_EQ(requestReader.getUserID(), "sophos-spl");
    EXPECT_EQ(requestReader.getPid(), -1);
    EXPECT_EQ(requestReader.getExecutablePath(), "");
    EXPECT_TRUE(requestReader.getScanInsideArchives());
}

TEST(TestScanMessages, CreateScanRequestSerialisationSetsMinusvalueCorrectlyForPid)
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname("/etc/fstab");
    requestBuilder.setScanInsideArchives(true);
    requestBuilder.setScanType(scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_DEMAND);
    requestBuilder.setUserID("sophos-spl");
    requestBuilder.setPid(-2);

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
    EXPECT_EQ(requestReader.getScanType(), scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_DEMAND);
    EXPECT_EQ(requestReader.getUserID(), "sophos-spl");
    EXPECT_EQ(requestReader.getPid(), -2);
    EXPECT_EQ(requestReader.getExecutablePath(), "");
    EXPECT_TRUE(requestReader.getScanInsideArchives());
}
TEST(TestScanMessages, CreateScanRequestObject)
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname("/etc/fstab");
    requestBuilder.setScanInsideArchives(false);
    requestBuilder.setScanType(scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_CLOSE);

    Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;

    auto scanRequest = std::make_unique<scan_messages::ScanRequest>(requestReader);

    EXPECT_EQ(scanRequest->path(), "/etc/fstab");
    EXPECT_EQ(scanRequest->getScanType(), scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_CLOSE);
    EXPECT_FALSE(scanRequest->scanInsideArchives());
}

TEST(TestScanMessages, ScanRequestReturnsScanTypeStrCorrectly)
{
    scan_messages::ClientScanRequest clientScanRequest;
    EXPECT_EQ("Unknown", scan_messages::getScanTypeAsStr(clientScanRequest.getScanType()));
    clientScanRequest.setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_OPEN);
    EXPECT_EQ("Open", scan_messages::getScanTypeAsStr(clientScanRequest.getScanType()));
    clientScanRequest.setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);
    EXPECT_EQ("Close-Write", scan_messages::getScanTypeAsStr(clientScanRequest.getScanType()));
    clientScanRequest.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    EXPECT_EQ("On Demand", scan_messages::getScanTypeAsStr(clientScanRequest.getScanType()));
    clientScanRequest.setScanType(scan_messages::E_SCAN_TYPE_SCHEDULED);
    EXPECT_EQ("Scheduled", scan_messages::getScanTypeAsStr(clientScanRequest.getScanType()));
    clientScanRequest.setScanType(scan_messages::E_SCAN_TYPE_MEMORY);
    EXPECT_EQ("Unknown", scan_messages::getScanTypeAsStr(clientScanRequest.getScanType()));
    clientScanRequest.setScanType(scan_messages::E_SCAN_TYPE_UNKNOWN);
    EXPECT_EQ("Unknown", scan_messages::getScanTypeAsStr(clientScanRequest.getScanType()));
}


TEST(TestScanMessages, ReuseScanRequestObject)
{
    auto scanRequest = std::make_unique<scan_messages::ScanRequest>();

    {
        ::capnp::MallocMessageBuilder message;
        Sophos::ssplav::FileScanRequest::Builder requestBuilder =
                message.initRoot<Sophos::ssplav::FileScanRequest>();
        requestBuilder.setPathname("/etc/fstab");
        requestBuilder.setScanInsideArchives(true);
        requestBuilder.setScanInsideImages(true);
        requestBuilder.setScanType(scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_OPEN);
        Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;
        scanRequest->resetRequest(requestReader);
    }

    EXPECT_EQ(scanRequest->path(), "/etc/fstab");
    EXPECT_EQ(scanRequest->getScanType(), scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_OPEN);
    EXPECT_TRUE(scanRequest->scanInsideArchives());
    EXPECT_TRUE(scanRequest->scanInsideImages());

    {
        ::capnp::MallocMessageBuilder message;
        Sophos::ssplav::FileScanRequest::Builder requestBuilder =
                message.initRoot<Sophos::ssplav::FileScanRequest>();
        requestBuilder.setPathname("/etc/shadow");
        requestBuilder.setScanInsideArchives(false);
        requestBuilder.setScanInsideImages(false);
        requestBuilder.setScanType(scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_UNKNOWN);
        Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;
        scanRequest->resetRequest(requestReader);
    }

    EXPECT_EQ(scanRequest->path(), "/etc/shadow");
    EXPECT_EQ(scanRequest->getScanType(), scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_UNKNOWN);
    EXPECT_FALSE(scanRequest->scanInsideArchives());
    EXPECT_FALSE(scanRequest->scanInsideImages());
}
