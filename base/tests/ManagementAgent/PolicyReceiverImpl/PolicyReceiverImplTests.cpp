/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <tests/ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <tests/ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h>
#include <modules/ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>
#include <modules/Common/TaskQueueImpl/TaskQueueImpl.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>

class PolicyReceiverImplTests : public ::testing::Test
{
public:

    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
    StrictMock<MockPluginManager> m_mockPluginManager;
};

TEST_F(PolicyReceiverImplTests, PolicyReceiverConstructorWithValidDataDoesNotThrow) // NOLINT
{
    std::string mcsDir("test/mcs");
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);

    EXPECT_NO_THROW(ManagementAgent::PolicyReceiverImpl::PolicyReceiverImpl policyReceiver(mcsDir, fakeQueue, m_mockPluginManager));

}

TEST_F(PolicyReceiverImplTests, receivedGetPolicyRequest_ResultsInPolicyTaskAddedToQeue)
{
    auto filesystemMock = new NiceMock<MockFileSystem>();
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string mcsDir("test/mcs");

    std::string appId = "AppID";
    std::string policyId = "25";

    std::vector<std::string> directoryFileList;
    directoryFileList.push_back(appId + "-" + policyId + "_policy.xml");

    EXPECT_CALL(*filesystemMock, listFiles(_)).WillOnce(Return(directoryFileList));

    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ManagementAgent::PolicyReceiverImpl::PolicyReceiverImpl policyReceiver(mcsDir, fakeQueue, m_mockPluginManager);
    policyReceiver.receivedGetPolicyRequest(appId, policyId);
    Common::TaskQueue::ITaskPtr task = fakeQueue->popTask();

    EXPECT_NE(task.get(),nullptr);

    Common::FileSystem::restoreFileSystem();
}

TEST_F(PolicyReceiverImplTests, receivedGetPolicyRequestWillApplyPolicy) //NOLINT
{
    auto filesystemMock = new NiceMock<MockFileSystem>();
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string mcsDir("test/mcs");

    std::string appId = "AppID";
    std::string policyId = "25";
    std::string policyFileName(appId + "-" + policyId + "_policy.xml");

    std::string fileContent("PolicyXml");

    std::vector<std::string> directoryFileList;
    directoryFileList.push_back(policyFileName);

    EXPECT_CALL(*filesystemMock, listFiles(_)).WillOnce(Return(directoryFileList));

    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ManagementAgent::PolicyReceiverImpl::PolicyReceiverImpl policyReceiver(mcsDir, fakeQueue, m_mockPluginManager);
    policyReceiver.receivedGetPolicyRequest(appId, policyId);
    Common::TaskQueue::ITaskPtr task = fakeQueue->popTask();

    EXPECT_NE(task.get(),nullptr);

    EXPECT_CALL(*filesystemMock, basename(_)).WillOnce(Return(policyFileName));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(fileContent));
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy(appId, fileContent)).WillOnce(Return(1));

    task->run();

    Common::FileSystem::restoreFileSystem();
}

