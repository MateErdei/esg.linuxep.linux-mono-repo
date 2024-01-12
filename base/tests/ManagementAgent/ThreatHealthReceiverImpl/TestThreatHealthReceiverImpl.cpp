// Copyright 2021-2024 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/StatusReceiverImpl/StatusReceiverImpl.h"
#include "ManagementAgent/ThreatHealthReceiverImpl/ThreatHealthReceiverImpl.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/TaskQueueImpl/FakeQueue.h"

class TestThreatHealthReceiverImpl : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestThreatHealthReceiverImpl, TestConstructorDoesNotThrow) // NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ASSERT_NO_THROW(ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl healthReceiver(fakeQueue));
}

TEST_F(TestThreatHealthReceiverImpl, ThreatHealthReceiverReceivesValidJsonAndCreatesRunnableTask) // NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl healthReceiver(fakeQueue);
    auto healthStatusSharedObj = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    std::string threatHealthJson = R"({"ThreatHealth": 123})";
    healthReceiver.receivedThreatHealth("pluginName", threatHealthJson, healthStatusSharedObj);

    auto task = fakeQueue->popTask();
    task->run();
    auto result = healthStatusSharedObj->generateHealthStatusXml();
    ASSERT_EQ(result.hasStatusChanged, true);
    ASSERT_EQ(
        result.statusXML,
        "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" activeHeartbeat=\"false\" "
        "activeHeartbeatUtmId=\"\"><item name=\"health\" value=\"123\" /><item name=\"admin\" value=\"1\" /><item name=\"threat\" value=\"123\" "
        "/></health>");
}

TEST_F(TestThreatHealthReceiverImpl, invalidThreatHealthJson) // NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl healthReceiver(fakeQueue);
    auto healthStatusSharedObj = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    std::string threatHealthJson = "this is not json";
    ASSERT_NO_THROW(healthReceiver.receivedThreatHealth("pluginName", threatHealthJson, healthStatusSharedObj));
}

TEST_F(TestThreatHealthReceiverImpl, unsetHealthStatusSharedObjPtrDoesNotCauseThrow) // NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl healthReceiver(fakeQueue);

    // not being initialised
    std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj;

    std::string threatHealthJson = R"({"ThreatHealth": 1})";
    ASSERT_NO_THROW(healthReceiver.receivedThreatHealth("pluginName", threatHealthJson, healthStatusSharedObj));
}
