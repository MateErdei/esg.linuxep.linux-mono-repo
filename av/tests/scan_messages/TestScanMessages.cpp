/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanRequest.capnp.h"

#include <scan_messages/ScanRequest.h>

#include <gtest/gtest.h>
#include <capnp/message.h>
#include <capnp/serialize.h>

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
