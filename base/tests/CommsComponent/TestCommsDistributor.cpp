/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include "CommsComponent/CommsDistributor.h"

class TestCommsDistributor : public LogInitializedTests
{};
TEST_F(TestCommsDistributor, testDistributorCanBeConstructed) // NOLINT
{
    const std::string path = "/tmp";
    const std::string filter = "filter";
    CommsComponent::MessageChannel messageChannel;
    CommsComponent::CommsDistributor distributor(path, filter, std::ref(messageChannel));
}

TEST_F(TestCommsDistributor, testdoRequestCanBeConstructed) // NOLINT
{
    const std::string filter = ".log";
    auto m_tempDir = Tests::TempDir::makeTempDir();
    std::string filepath = m_tempDir->dirPath();

    CommsComponent::MessageChannel messageChannel;
    CommsComponent::CommsDistributor distributor(filepath, filter, std::ref(messageChannel));
    m_tempDir->createFile("test.log","blah");

    std::thread handlerThread(&CommsComponent::CommsDistributor::handleRequestsAndResponses, &distributor);

    distributor.stop();
}
