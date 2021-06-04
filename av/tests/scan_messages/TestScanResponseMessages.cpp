/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

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
        m_scanResponse.addDetection(m_filePath, m_threatName);
    }

    std::string m_filePath = "/tmp/eicar.com";
    std::string m_threatName = "EICAR-AV-TEST";
    std::string m_fullScanResult = "";

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
    EXPECT_EQ(deSerialisedData.getFullScanResult(), m_fullScanResult);
}