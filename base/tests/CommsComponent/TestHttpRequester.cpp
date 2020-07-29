
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

    std::cout << CommsComponent::HttpRequester::generateId("requester") << std::endl;
    std::cout << CommsComponent::HttpRequester::generateId("requester") << std::endl;
    std::cout << CommsComponent::HttpRequester::generateId("requester") << std::endl;
}

//        static std::optional<Common::HttpSender::HttpResponse> triggerRequest(const std::string& requesterName, Common::HttpSender::RequestConfig&& , std::string && body);
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
    EXPECT_CALL(*filesystemMock,writeFile(m_requestTempDir->absPath("request_testRequester_1591790400_0_body"),"testBody"));
    EXPECT_CALL(*filesystemMock,writeFileAtomically(m_requestTempDir->absPath("request_testRequester_1591790400_0.json"),_,responseTempDirPath)).WillOnce(Invoke([&testExecutionSynchronizer](const std::string&, const std::string&, const std::string&) { testExecutionSynchronizer.notify(); }));
    std::string requesterName = "testRequester";
    std::string requestJson = R"({"requestType": "POST"})";
//    CommsComponent::HttpRequester::triggerRequest(requesterName, CommsComponent::CommsMsg::requestConfigFromJson(requestJson), "testBody");
//    std::thread requesterThread(CommsComponent::HttpRequester::triggerRequest, requesterName, CommsComponent::CommsMsg::requestConfigFromJson(requestJson), "testbody");
    auto requesterThread = std::async(std::launch::async, [requesterName, requestJson](){ return CommsComponent::HttpRequester::triggerRequest(requesterName, CommsComponent::CommsMsg::requestConfigFromJson(requestJson), "testBody");});

    EXPECT_TRUE(testExecutionSynchronizer.waitfor(500));

    scopedReplaceFileSystem.reset();
    std::string expectedResponseBaseName = "response_testRequester_1591790400_0.json";
    Path expectedResponsePath = m_responseTempDir->absPath(expectedResponseBaseName);
    Path tempPath = m_responseTempDir->absPath("temp_response");
    std::string responseContent = R"({"httpCode": 1})";

    fileSystem()->writeFile(tempPath, responseContent);
    fileSystem()->moveFile(tempPath, expectedResponsePath);

    auto response = requesterThread.get();
    EXPECT_EQ(response.httpCode, 1);
}
