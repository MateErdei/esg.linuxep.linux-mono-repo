/******************************************************************************************************

Copyright 2019-2021, Sophos Limited.  All rights reserved.

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
        m_scanResponse.addDetection(m_filePath, m_threatName, m_sha256);
    }

    std::string m_filePath = "/tmp/eicar.com";
    std::string m_threatName = "EICAR-AV-TEST";
    std::string m_sha256 = "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def";

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

    ::capnp::List<Sophos::ssplav::FileScanResponse::Detection>::Reader detections = deSerialisedData.getDetections();

    EXPECT_EQ(detections[0].getThreatName(), m_threatName);
    EXPECT_EQ(detections[0].getSha256(), m_sha256);
    EXPECT_EQ(detections[0].getFilePath(), m_filePath);
}