
/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommsComponent/HttpRequester.h"
#include <gtest/gtest.h>
#include <modules/CommsComponent/CommsMsg.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/DirectoryWatcher/IDirectoryWatcherException.h>
#include <modules/Common/UtilityImpl/TimeUtils.h>
#include <tests/Common/Helpers/FakeTimeUtils.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/TempDir.h>
#include <thread>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>

class TestHttpRequester : public LogInitializedTests
{};

std::time_t t_20190501T13h{ 1556712000 };
std::time_t t_20200610T12h{ 1591790400 };

TEST_F(TestHttpRequester, testGenerateId)
{
    bool stop{ false };
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    auto id1 = CommsComponent::HttpRequester::generateId("requester");
    auto id2 = CommsComponent::HttpRequester::generateId("requester2");
    auto id3 = CommsComponent::HttpRequester::generateId("requester3");

    EXPECT_NE(id1, id2);
    EXPECT_NE(id1, id3);
    EXPECT_NE(id2, id3);
    EXPECT_THAT(id1, ::testing::StartsWith("requester_1591790400_"));
    EXPECT_THAT(id2, ::testing::StartsWith("requester2_1591790400_"));
    EXPECT_THAT(id3, ::testing::StartsWith("requester3_1591790400_"));
}

TEST_F(TestHttpRequester, testTriggerRequest)
{
    bool stop{ false };
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    auto m_requestTempDir = Tests::TempDir::makeTempDir();
    auto m_responseTempDir = Tests::TempDir::makeTempDir();
    std::string requestTempDirPath = m_requestTempDir->dirPath();
    std::string responseTempDirPath = m_responseTempDir->dirPath();

    MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
    MockedApplicationPathManager& mock(*mockAppManager);
    ON_CALL(mock, getCommsRequestDirPath()).WillByDefault(Return(requestTempDirPath));
    ON_CALL(mock, getCommsResponseDirPath()).WillByDefault(Return(responseTempDirPath));
    ON_CALL(mock, getTempPath()).WillByDefault(Return(responseTempDirPath));
    Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));


    auto filesystemMock = new NiceMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    Tests::TestExecutionSynchronizer testExecutionSynchronizer(1);
    std::stringstream expectedRequestJsonSubstrRegex;
    expectedRequestJsonSubstrRegex << requestTempDirPath << R"(/request_testRequester_1591790400_[0-9]*\.json)";
    std::stringstream expectedRequestBodySubstrRegex;
    expectedRequestBodySubstrRegex << requestTempDirPath << R"(/request_testRequester_1591790400_[0-9]*_body)";

    std::string actualRequestJsonPath;
    EXPECT_CALL(*filesystemMock, writeFile( MatchesRegex(expectedRequestBodySubstrRegex.str()),"testBody")).WillOnce(SaveArg<0>(&actualRequestJsonPath));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(MatchesRegex(expectedRequestJsonSubstrRegex.str()),_,responseTempDirPath)).WillOnce(Invoke([&testExecutionSynchronizer](const std::string&, const std::string&, const std::string&) { testExecutionSynchronizer.notify(); }));
    std::string requesterName = "testRequester";
    std::string requestJson = R"({"requestType": "POST"})";
    auto requesterThread = std::async(std::launch::async, [requesterName, requestJson](){ return CommsComponent::HttpRequester::triggerRequest(
            requesterName, CommsComponent::CommsMsg::requestConfigFromJson(requestJson), "testBody",
            std::chrono::milliseconds(500));});

    EXPECT_TRUE(testExecutionSynchronizer.waitfor(500));

    scopedReplaceFileSystem.reset();
    std::stringstream expectedResponseBaseNameSS;
    expectedResponseBaseNameSS << "response_" << Common::FileSystem::basename(actualRequestJsonPath).substr(8,actualRequestJsonPath.size()-8);
    std::string expectedResponseBaseName = expectedResponseBaseNameSS.str();
    Path expectedResponsePath = m_responseTempDir->absPath(expectedResponseBaseName);
    Path tempPath = m_responseTempDir->absPath("temp_response");
    std::string responseContent = R"({"httpCode": 1})";

    fileSystem()->writeFile(tempPath, responseContent);
    fileSystem()->moveFile(tempPath, expectedResponsePath);

    auto response = requesterThread.get();
    EXPECT_EQ(response.httpCode, 1);
}

TEST_F(TestHttpRequester, testRequesterDealsWithInvalidResponse)
{
    bool stop{ false };
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    auto m_requestTempDir = Tests::TempDir::makeTempDir();
    auto m_responseTempDir = Tests::TempDir::makeTempDir();
    std::string requestTempDirPath = m_requestTempDir->dirPath();
    std::string responseTempDirPath = m_responseTempDir->dirPath();

    MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
    MockedApplicationPathManager& mock(*mockAppManager);
    ON_CALL(mock, getCommsRequestDirPath()).WillByDefault(Return(requestTempDirPath));
    ON_CALL(mock, getCommsResponseDirPath()).WillByDefault(Return(responseTempDirPath));
    ON_CALL(mock, getTempPath()).WillByDefault(Return(responseTempDirPath));
    Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));


    auto filesystemMock = new NiceMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    Tests::TestExecutionSynchronizer testExecutionSynchronizer(1);
    std::stringstream expectedRequestJsonSubstrRegex;
    expectedRequestJsonSubstrRegex << requestTempDirPath << R"(/request_testRequester_1591790400_[0-9]*\.json)";
    std::stringstream expectedRequestBodySubstrRegex;
    expectedRequestBodySubstrRegex << requestTempDirPath << R"(/request_testRequester_1591790400_[0-9]*_body)";

    std::string actualRequestJsonPath;
    EXPECT_CALL(*filesystemMock, writeFile( MatchesRegex(expectedRequestBodySubstrRegex.str()),"testBody")).WillOnce(SaveArg<0>(&actualRequestJsonPath));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(MatchesRegex(expectedRequestJsonSubstrRegex.str()),_,responseTempDirPath)).WillOnce(Invoke([&testExecutionSynchronizer](const std::string&, const std::string&, const std::string&) { testExecutionSynchronizer.notify(); }));
    std::string requesterName = "testRequester";
    std::string requestJson = R"({"requestType": "POST"})";
    auto requesterThread = std::async(std::launch::async, [requesterName, requestJson](){ return CommsComponent::HttpRequester::triggerRequest(
            requesterName, CommsComponent::CommsMsg::requestConfigFromJson(requestJson), "testBody",
            std::chrono::milliseconds(500));});

    EXPECT_TRUE(testExecutionSynchronizer.waitfor(500));

    scopedReplaceFileSystem.reset();
    std::stringstream expectedResponseBaseNameSS;
    expectedResponseBaseNameSS << "response_" << Common::FileSystem::basename(actualRequestJsonPath).substr(8,actualRequestJsonPath.size()-8);
    std::string expectedResponseBaseName = expectedResponseBaseNameSS.str();
    Path expectedResponsePath = m_responseTempDir->absPath(expectedResponseBaseName);
    Path tempPath = m_responseTempDir->absPath("temp_response");
    std::string responseContent = R"({"httpCode": "notanint"})";

    fileSystem()->writeFile(tempPath, responseContent);
    fileSystem()->moveFile(tempPath, expectedResponsePath);

    EXPECT_THROW(requesterThread.get(), CommsComponent::InvalidCommsMsgException);
}

TEST_F(TestHttpRequester, testRequesterHandlesNoResponeFileBack)
{
    bool stop{ false };
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    auto m_requestTempDir = Tests::TempDir::makeTempDir();
    auto m_responseTempDir = Tests::TempDir::makeTempDir();
    std::string requestTempDirPath = m_requestTempDir->dirPath();
    std::string responseTempDirPath = m_responseTempDir->dirPath();

    MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
    MockedApplicationPathManager& mock(*mockAppManager);
    ON_CALL(mock, getCommsRequestDirPath()).WillByDefault(Return(requestTempDirPath));
    ON_CALL(mock, getCommsResponseDirPath()).WillByDefault(Return(responseTempDirPath));
    ON_CALL(mock, getTempPath()).WillByDefault(Return(responseTempDirPath));
    Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));


    auto filesystemMock = new NiceMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    Tests::TestExecutionSynchronizer testExecutionSynchronizer(1);
    std::stringstream expectedRequestJsonSubstrRegex;
    expectedRequestJsonSubstrRegex << requestTempDirPath << R"(/request_testRequester_1591790400_[0-9]*\.json)";
    std::stringstream expectedRequestBodySubstrRegex;
    expectedRequestBodySubstrRegex << requestTempDirPath << R"(/request_testRequester_1591790400_[0-9]*_body)";

    std::string actualRequestJsonPath;
    EXPECT_CALL(*filesystemMock, writeFile( MatchesRegex(expectedRequestBodySubstrRegex.str()),"testBody")).WillOnce(SaveArg<0>(&actualRequestJsonPath));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(MatchesRegex(expectedRequestJsonSubstrRegex.str()),_,responseTempDirPath)).WillOnce(Invoke([&testExecutionSynchronizer](const std::string&, const std::string&, const std::string&) { testExecutionSynchronizer.notify(); }));
    std::string requesterName = "testRequester";
    std::string requestJson = R"({"requestType": "POST"})";
    auto requesterThread = std::async(std::launch::async, [requesterName, requestJson](){ return CommsComponent::HttpRequester::triggerRequest(
            requesterName, CommsComponent::CommsMsg::requestConfigFromJson(requestJson), "testBody",
            std::chrono::milliseconds(500));});

    EXPECT_TRUE(testExecutionSynchronizer.waitfor(500));

    scopedReplaceFileSystem.reset();
    std::stringstream expectedResponseBaseNameSS;
    expectedResponseBaseNameSS << "NotTheResponseWeAreLookingFor.json";
    std::string expectedResponseBaseName = expectedResponseBaseNameSS.str();
    Path expectedResponsePath = m_responseTempDir->absPath(expectedResponseBaseName);
    Path tempPath = m_responseTempDir->absPath("temp_response");
    std::string responseContent = R"({"httpCode": 1})";


    fileSystem()->writeFile(tempPath, responseContent);
    fileSystem()->moveFile(tempPath, expectedResponsePath);

    EXPECT_THROW(requesterThread.get(), CommsComponent::HttpRequesterException);
}