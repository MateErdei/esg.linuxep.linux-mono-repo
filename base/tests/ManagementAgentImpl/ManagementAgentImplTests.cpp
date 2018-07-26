/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <tests/ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <modules/ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>

namespace
{
    class TestManagementAgent
            : public ManagementAgent::ManagementAgentImpl::ManagementAgentMain
    {
    public:
        void initialiseWrapper(ManagementAgent::PluginCommunication::IPluginManager& pluginManager)
        {
            initialise(pluginManager);
        }
    };

    class ManagementAgentImplTests : public ::testing::Test
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

    TEST_F(ManagementAgentImplTests, ManagementAgentMainConstructorWithValidDataDoesNotThrow) // NOLINT
    {
        EXPECT_NO_THROW(ManagementAgent::ManagementAgentImpl::ManagementAgentMain managementAgent());
    }

    TEST_F(ManagementAgentImplTests, initialiseWillNotThrow)
    {

        //TestManagementAgent managementAgent;

        //EXPECT_NO_THROW(managementAgent.initialiseWrapper(m_mockPluginManager));
    }
}


