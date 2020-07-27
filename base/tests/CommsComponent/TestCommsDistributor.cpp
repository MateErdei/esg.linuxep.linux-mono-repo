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
    m_requestTempDir->createFile("request_firstTest_body", "body contents for the first test");
    m_requestTempDir->createFile("request_TestItASecondTime_body", "these are the contents we send in the second test");
    std::string source1 = m_requestTempDir->absPath("testFile1");
    std::string source2 = m_requestTempDir->absPath("testFile2");
    std::string requestFilePath1 = m_requestTempDir->absPath("request_firstTest.json");
    std::string requestBodyPath1 = m_requestTempDir->absPath("request_firstTest_body");
    std::string requestFilePath2 = m_requestTempDir->absPath("request_TestItASecondTime.json");
    std::string requestBodyPath2 = m_requestTempDir->absPath("request_TestItASecondTime_body");
    auto fileSystem = Common::FileSystem::fileSystem();

    // start handling responses/requests
    std::thread handlerThread(&CommsComponent::CommsDistributor::handleRequestsAndResponses, &distributor);

    EXPECT_CALL(distributor, forwardResponse(serialisedResponseJson)).Times(2);
    messageChannel.push(serialisedResponseJson);
    messageChannel.push(serialisedResponseJson);

    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    EXPECT_CALL(mockOthersideApi, pushMessage(HasSubstr("body contents for the first test"))).Times(1).WillOnce(Invoke([&testExecutionSynchronizer](const std::string&) { testExecutionSynchronizer.notify(); }));
    EXPECT_CALL(mockOthersideApi, pushMessage(HasSubstr("these are the contents we send in the second test"))).Times(1).WillOnce(Invoke([&testExecutionSynchronizer](const std::string&) { testExecutionSynchronizer.notify(); }));
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

TEST_F(TestCommsDistributor, testDistributorHandlesIncorrectRequests) // NOLINT
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
    m_requestTempDir->createFile("notarequest_firstTest_body", "body contents for the first test");
    std::string source1 = m_requestTempDir->absPath("testFile1");
    std::string badRequestFilePath1 = m_requestTempDir->absPath("notarequest_firstTest.json");
    std::string requestBodyPath1 = m_requestTempDir->absPath("notarequest_firstTest_body");
    auto fileSystem = Common::FileSystem::fileSystem();

    // start handling responses/requests
    std::thread handlerThread(&CommsComponent::CommsDistributor::handleRequestsAndResponses, &distributor);

    // push empty message
    messageChannel.push("");
    // expect no response forwarding as we do not handle empty responses
    EXPECT_CALL(distributor, forwardResponse(_)).Times(0);

    EXPECT_CALL(mockOthersideApi, pushMessage(HasSubstr("body contents for the first test"))).Times(0);
    fileSystem->moveFile(source1, badRequestFilePath1);

    EXPECT_CALL(mockOthersideApi, notifyOtherSideAndClose());

    // stop the distributor
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    distributor.stop();
    handlerThread.join();

    // these files should have been cleaned up by the distributor after being used
    EXPECT_FALSE(fileSystem->isFile(badRequestFilePath1));
    // we cannot remove the body in this case as we expect it to fit: request_<id>_body
    EXPECT_TRUE(fileSystem->isFile(requestBodyPath1));
}

TEST_F(TestCommsDistributor, testGetExpectedRequestBodyBaseNameFromId)
{
    std::string id = "id";
    std::string prepender = "request_";
    std::string appender = "_body";
    EXPECT_EQ(
            CommsComponent::CommsDistributor::getExpectedRequestBodyBaseNameFromId(id, prepender, appender),
            "request_id_body");

    id = "differentid";
    prepender = "notarequest_";
    appender = "_notabody";
    EXPECT_EQ(
            CommsComponent::CommsDistributor::getExpectedRequestBodyBaseNameFromId(id, prepender, appender),
            "notarequest_differentid_notabody");
}

TEST_F(TestCommsDistributor, testGetIdFromRequestBaseName)
{
    std::string baseName = "request_id.json";
    std::string prepender = "request_";
    std::string appender = ".json";
    EXPECT_EQ(
            CommsComponent::CommsDistributor::getIdFromRequestBaseName(baseName, prepender, appender),
            "id");

    baseName = "request_id.json";
    prepender = "requestbutlonger_";
    appender = ".jsonbutlonger";
    EXPECT_THROW(
            CommsComponent::CommsDistributor::getIdFromRequestBaseName(baseName, prepender, appender),
            std::runtime_error);
}