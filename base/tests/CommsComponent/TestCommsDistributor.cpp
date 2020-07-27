/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <modules/CommsComponent/AsyncMessager.h>
#include "MockOtherSideApi.h"
#include "MockCommsDistributor.h"
#include <utility>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>
#include <modules/CommsComponent/CommsMsg.h>
#include "CommsComponent/CommsDistributor.h"
#include "Common/FileSystem/IFileSystem.h"

class TestCommsDistributor : public LogInitializedTests
{};

std::string getSerializedBasicResponse()
{
    std::string responseJson = R"({"httpCode": 3})";

    Common::HttpSender::HttpResponse response = CommsComponent::CommsMsg::httpResponseFromJson(responseJson);
    CommsComponent::CommsMsg responseMsg;
    int responseId = 1;
    responseMsg.id = responseId;
    responseMsg.content = response;
    return CommsComponent::CommsMsg::serialize(responseMsg);
}

TEST_F(TestCommsDistributor, testDistributorCanBeConstructed) // NOLINT
{
    const std::string requestPath = "/tmp";
    const std::string filter = "filter";
    const std::string responsePath = "/tmp";
    CommsComponent::MessageChannel messageChannel;

    MockOtherSideApi mockOthersideApi{};
    CommsComponent::CommsDistributor distributor(requestPath, filter, responsePath, messageChannel, mockOthersideApi);
}

TEST_F(TestCommsDistributor, testDistributorHandlesRequestFilesAndResponses) // NOLINT
{
    CommsComponent::MessageChannel messageChannel;
    auto m_requestTempDir = Tests::TempDir::makeTempDir();
    auto m_responseTempDir = Tests::TempDir::makeTempDir();
    const std::string filter = ".json";
    std::string requestTempDirPath = m_requestTempDir->dirPath();
    std::string responseTempDirPath = m_responseTempDir->dirPath();

    MockOtherSideApi mockOthersideApi{};

    //TODO LINUXDAR-1954 this mock will no longer be neccessary once this tickets work has been done
    // please remove the mock and adjust this test to expect the proper response/response body file to be created
    MockCommsDistributor distributor(requestTempDirPath, filter, responseTempDirPath, messageChannel,
                                                 mockOthersideApi);

    std::string requestJson = R"({"requestType": "POST"})";
    std::string serialisedResponseJson = getSerializedBasicResponse();

    m_requestTempDir->createFile("testFile1", requestJson);
    m_requestTempDir->createFile("testFile2", requestJson);
    m_requestTempDir->createFile("request_test1_body", "body contents for test1");
    m_requestTempDir->createFile("request_test2_body", "body contents for test2");
    std::string source1 = m_requestTempDir->absPath("testFile1");
    std::string source2 = m_requestTempDir->absPath("testFile2");
    std::string requestFilePath1 = m_requestTempDir->absPath("request_test1.json");
    std::string requestBodyPath1 = m_requestTempDir->absPath("request_test1_body");
    std::string requestFilePath2 = m_requestTempDir->absPath("request_test2.json");
    std::string requestBodyPath2 = m_requestTempDir->absPath("request_test2_body");
    auto fileSystem = Common::FileSystem::fileSystem();

    // start handling responses/requests
    std::thread handlerThread(&CommsComponent::CommsDistributor::handleRequestsAndResponses, &distributor);

    EXPECT_CALL(distributor, forwardResponse(serialisedResponseJson)).Times(2);
    messageChannel.push(serialisedResponseJson);
    messageChannel.push(serialisedResponseJson);

    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    EXPECT_CALL(mockOthersideApi, pushMessage(HasSubstr("body contents for test1"))).Times(1).WillOnce(Invoke([&testExecutionSynchronizer](const std::string&) { testExecutionSynchronizer.notify(); }));
    EXPECT_CALL(mockOthersideApi, pushMessage(HasSubstr("body contents for test2"))).Times(1).WillOnce(Invoke([&testExecutionSynchronizer](const std::string&) { testExecutionSynchronizer.notify(); }));
    fileSystem->moveFile(source1, requestFilePath1);
    fileSystem->moveFile(source2, requestFilePath2);

    EXPECT_CALL(mockOthersideApi, notifyOtherSideAndClose());

    testExecutionSynchronizer.waitfor();
    // stop the distributor
    distributor.stop();
    handlerThread.join();

    // these files should have been cleaned up by the distributor after being used
    EXPECT_FALSE(fileSystem->isFile(requestFilePath1));
    EXPECT_FALSE(fileSystem->isFile(requestBodyPath1));
    EXPECT_FALSE(fileSystem->isFile(requestFilePath2));
    EXPECT_FALSE(fileSystem->isFile(requestBodyPath2));
}