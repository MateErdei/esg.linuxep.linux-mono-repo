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
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include "CommsComponent/CommsDistributor.h"
#include "Common/FileSystem/IFileSystem.h"
#include "CommsMsgUtils.h"

class TestCommsDistributor : public LogInitializedTests
{};

bool checkSerializedMessageIsEquivalentToResponseJsonContent(std::string serializedString, std::string jsonContent)
{
    std::string deserializedJson = CommsMsg::toJson(std::get<Common::HttpSender::HttpResponse>(CommsComponent::CommsMsg::fromString(serializedString).content));
    return deserializedJson == jsonContent;


}

std::string getSerializedBasicResponse(std::string responseId)
{
    std::string responseJson = R"({"httpCode": 3})";

    Common::HttpSender::HttpResponse response = CommsComponent::CommsMsg::httpResponseFromJson(responseJson);
    CommsComponent::CommsMsg responseMsg;
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

    MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
    MockedApplicationPathManager& mock(*mockAppManager);
    ON_CALL(mock, getTempPath()).WillByDefault(Return(requestTempDirPath));
    Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

    CommsComponent::CommsDistributor distributor(requestTempDirPath, filter, responseTempDirPath, messageChannel,
                                                 mockOthersideApi);

    std::string requestJson = R"({"requestType": "POST"})";
    std::string serialisedResponseJson1 = getSerializedBasicResponse("1");
    std::string serialisedResponseJson2 = getSerializedBasicResponse("2");

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
    std::string expectedResponsePath1 = m_responseTempDir->absPath("response_1.json");
    std::string expectedResponsePath2 = m_responseTempDir->absPath("response_2.json");
    auto fileSystem = Common::FileSystem::fileSystem();

    // start handling responses/requests
    std::thread handlerThread(&CommsComponent::CommsDistributor::handleRequestsAndResponses, &distributor);

//    EXPECT_CALL(distributor, forwardResponse(serialisedResponseJson)).Times(2);
    messageChannel.push(serialisedResponseJson1);
    messageChannel.push(serialisedResponseJson2);

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
    EXPECT_EQ(fileSystem->listFiles(requestTempDirPath).size(), 0);

    EXPECT_TRUE(fileSystem->isFile(expectedResponsePath1));
    EXPECT_TRUE(checkSerializedMessageIsEquivalentToResponseJsonContent(serialisedResponseJson1, fileSystem->readFile(expectedResponsePath1)));
    EXPECT_TRUE(fileSystem->isFile(expectedResponsePath2));
    EXPECT_TRUE(checkSerializedMessageIsEquivalentToResponseJsonContent(serialisedResponseJson2, fileSystem->readFile(expectedResponsePath2)));
}

TEST_F(TestCommsDistributor, testDistributorHandlesIncorrectRequests) // NOLINT
{
    CommsComponent::MessageChannel messageChannel;
    const std::string filter = ".json";
    auto m_requestTempDir = Tests::TempDir::makeTempDir();
    auto m_responseTempDir = Tests::TempDir::makeTempDir();
    std::string requestTempDirPath = m_requestTempDir->dirPath();
    std::string responseTempDirPath = m_responseTempDir->dirPath();

    MockOtherSideApi mockOthersideApi{};

    MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
    MockedApplicationPathManager& mock(*mockAppManager);
    ON_CALL(mock, getTempPath()).WillByDefault(Return(requestTempDirPath));
    Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

    CommsComponent::CommsDistributor distributor(requestTempDirPath, filter, responseTempDirPath, messageChannel,
                                     mockOthersideApi);

    std::string requestJson = R"({"requestType": "POST"})";
    std::string serialisedResponseJson = getSerializedBasicResponse("1");

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
    EXPECT_EQ(fileSystem->listFiles(responseTempDirPath).size(), 0);
    // only file is the body we fail to clean up
    EXPECT_EQ(fileSystem->listFiles(requestTempDirPath).size(), 1);
}

TEST_F(TestCommsDistributor, testGetExpectedRequestBodyBaseNameFromId)
{
    std::string id = "id";
    EXPECT_EQ(
            CommsComponent::CommsDistributor::getExpectedRequestBodyBaseNameFromId(id),
            "request_id_body");

    id = "differentid";
    EXPECT_EQ(
            CommsComponent::CommsDistributor::getExpectedRequestBodyBaseNameFromId(id),
            "request_differentid_body");
}

TEST_F(TestCommsDistributor, testGetExpectedRequestJsonBaseNameFromId)
{
    std::string id = "id";
    EXPECT_EQ(
            CommsComponent::CommsDistributor::getExpectedRequestJsonBaseNameFromId(id),
            "request_id.json");

    id = "differentid";
    EXPECT_EQ(
            CommsComponent::CommsDistributor::getExpectedRequestJsonBaseNameFromId(id),
            "request_differentid.json");
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

TEST_F(TestCommsDistributor, testGetSerializedRequest)
{
    std::string requestContents = R"({
 "requestType": "POST",
 "server": "domain.com",
 "resourcePath": "/index.html",
 "port": "335",
 "certPath" : "certPath1",
 "headers": [
  "header1",
  "header2"
 ]
})";

    Common::HttpSender::RequestConfig expectedRequest = CommsComponent::CommsMsg::requestConfigFromJson(requestContents);
    std::string bodyContents = "body contents";
    expectedRequest.setData(bodyContents);
    std::string id = "test";
    std::string serializedRequest = CommsComponent::CommsDistributor::getSerializedRequest(requestContents, bodyContents, id);
    CommsComponent::CommsMsg msg = CommsComponent::CommsMsg::fromString(serializedRequest);
    EXPECT_TRUE(std::holds_alternative<Common::HttpSender::RequestConfig>(msg.content));
    std::stringstream s;
    EXPECT_TRUE(requestAreEquivalent(s, expectedRequest, std::get<Common::HttpSender::RequestConfig>(msg.content)));
    EXPECT_EQ(id, msg.id);
}