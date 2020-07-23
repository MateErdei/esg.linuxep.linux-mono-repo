/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include "CommsComponent/CommsDistributor.h"
#include "Common/FileSystem/IFileSystem.h"

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
    std::string  source = m_tempDir->absPath("test.log");
    std::string  destination = m_tempDir->absPath("test1.log");
    auto fileSystem = Common::FileSystem::fileSystem();


    std::thread handlerThread(&CommsComponent::CommsDistributor::handleRequestsAndResponses, &distributor);
    messageChannel.push("request_test");
    fileSystem->moveFile(source,destination);
    std::this_thread::sleep_for(std::chrono::milliseconds (100));
    distributor.stop();
    handlerThread.join();
}
