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
    //std::string mcsDir("test/mcs");
    //std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue = std::make_shared<Common::TaskQueueImpl::TaskQueueImpl>();

    //EXPECT_NO_THROW(ManagementAgent::PolicyReceiverImpl::PolicyReceiverImpl policyReceiver(mcsDir, taskQueue, m_mockPluginManager));

}

