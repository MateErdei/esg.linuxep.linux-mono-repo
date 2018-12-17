/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <Common/TestHelpers/FileSystemReplaceAndRestore.h>
#include <Common/TestHelpers/MockFileSystem.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>
#include <tests/ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h>
#include <tests/Common/Logging/TestConsoleLoggingSetup.h>

class PolicyReceiverImplTests : public ::testing::Test
{
public:

    PolicyReceiverImplTests() = default;

    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
    StrictMock<MockPluginManager> m_mockPluginManager;

private:
    TestLogging::TestConsoleLoggingSetup m_loggingSetup;
};



TEST_F(PolicyReceiverImplTests, PolicyReceiverConstructorWithValidDataDoesNotThrow) // NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);

    EXPECT_NO_THROW( //NOLINT
            ManagementAgent::PolicyReceiverImpl::PolicyReceiverImpl policyReceiver(fakeQueue, m_mockPluginManager));

}

TEST_F(PolicyReceiverImplTests, receivedGetPolicyRequest_ResultsInPolicyTaskAddedToQeue) //NOLINT
{
    auto filesystemMock = new NiceMock<MockFileSystem>();
    Common::TestHelpers::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string appId = "AppID";
    std::string policyId = "25";

    std::vector<std::string> directoryFileList;
    directoryFileList.push_back(appId + "-" + policyId + "_policy.xml");

    EXPECT_CALL(*filesystemMock, listFiles(_)).WillOnce(Return(directoryFileList));

    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ManagementAgent::PolicyReceiverImpl::PolicyReceiverImpl policyReceiver(fakeQueue, m_mockPluginManager);
    policyReceiver.receivedGetPolicyRequest(appId);
    Common::TaskQueue::ITaskPtr task = fakeQueue->popTask();

    EXPECT_NE(task.get(),nullptr);

    Common::TestHelpers::restoreFileSystem();
}

TEST_F(PolicyReceiverImplTests, receivedGetPolicyRequestWillApplyPolicy) //NOLINT
{
    auto filesystemMock = new NiceMock<MockFileSystem>();
    Common::TestHelpers::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string appId = "AppID";
    std::string policyId = "25";
    std::string policyFileName(appId + "-" + policyId + "_policy.xml");

    std::string fileContent("PolicyXml");

    std::vector<std::string> directoryFileList;
    directoryFileList.push_back(policyFileName);

    EXPECT_CALL(*filesystemMock, listFiles(_)).WillOnce(Return(directoryFileList));

    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ManagementAgent::PolicyReceiverImpl::PolicyReceiverImpl policyReceiver(fakeQueue, m_mockPluginManager);
    policyReceiver.receivedGetPolicyRequest(appId);
    Common::TaskQueue::ITaskPtr task = fakeQueue->popTask();

    EXPECT_NE(task.get(),nullptr);

    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(fileContent));
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy(appId, fileContent)).WillOnce(Return(1));

    task->run();

    Common::TestHelpers::restoreFileSystem();
}

