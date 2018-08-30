/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include <tests/ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>

#include <ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>
#include <ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <ManagementAgent/LoggerImpl/LoggingSetup.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <modules/ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <tests/Common/TestHelpers/TempDir.h>
#include <future>
#include <modules/UpdateSchedulerImpl/LoggingSetup.h>
#include <modules/Common/PluginApi/IPluginResourceManagement.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <modules/UpdateSchedulerImpl/SchedulerPluginCallback.h>
#include <modules/Common/UtilityImpl/StringUtils.h>

namespace
{

    static std::string updatePolicyWithProxy{R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="1"/>
  <AUConfig platform="Linux">
    <sophos_address address="http://es-web.sophos.com/update"/>
    <primary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="Clear" UserPassword="54m5ung" UserName="QA940267" UseSophos="true" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="2" ProxyUserPassword="CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY=" ProxyUserName="TestUser" ProxyPortNumber="8080" ProxyAddress="uk-abn-wpan-1.green.sophos" AllowLocalConfig="false"/>
    </primary_location>
    <secondary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </secondary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="40" DetectDialUp="false"/>
    <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
    <bootstrap Location="" UsePrimaryServerAddress="true"/>
    <cloud_subscription RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="9"/>
    </cloud_subscriptions>
    <delay_supplements enabled="true"/>
  </AUConfig>
  <Features>
    <Feature id="APPCNTRL"/>
    <Feature id="AV"/>
    <Feature id="CORE"/>
    <Feature id="DLP"/>
    <Feature id="DVCCNTRL"/>
    <Feature id="EFW"/>
    <Feature id="HBT"/>
    <Feature id="MTD"/>
    <Feature id="NTP"/>
    <Feature id="SAV"/>
    <Feature id="SDU"/>
    <Feature id="WEBCNTRL"/>
  </Features>
  <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
  <customer id="9972e4cf-dba3-e4ab-19dc-77619acac988"/>
</AUConfigurations>
)sophos"};

    static std::string updateAction{R"sophos(<?xml version='1.0'?>
<action type="sophos.mgt.action.ALCForceUpdate"/>)sophos"};


    class TestManagementAgent
            : public ManagementAgent::ManagementAgentImpl::ManagementAgentMain
    {
    public:
        TestManagementAgent() = default;


        int runWrapper(std::chrono::seconds duration)
        {
            ManagementAgent::LoggerImpl::LoggingSetup loggerSetup(1);

            std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager> pluginManager = std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager>(
                    new ManagementAgent::PluginCommunicationImpl::PluginManager());

            initialise(*pluginManager);
            auto futureRunner = std::async(std::launch::async, [this]() {
                return run();
            });
            futureRunner.wait_for(duration);
            test_request_stop();
            return futureRunner.get();
        }

        Common::TaskQueue::ITaskQueueSharedPtr getTaskQueue()
        {
            return m_taskQueue;
        }
    };

    class FakeSchedulerPlugin
    {
    public:
        FakeSchedulerPlugin():
                logging(1),
                resourceManagement(Common::PluginApi::createPluginResourceManagement()),
                queueTask(std::make_shared<UpdateScheduler::SchedulerTaskQueue>()),
                sharedPluginCallBack(std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(queueTask)),
                baseService( resourceManagement->createPluginAPI("updatescheduler", sharedPluginCallBack))
        {

        }
        UpdateSchedulerImpl::LoggingSetup logging;
        std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement;
        std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> queueTask;
        std::shared_ptr<UpdateSchedulerImpl::SchedulerPluginCallback> sharedPluginCallBack;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;
        bool queue_has_policy()
        {
            return  queue_has_all_tasks({UpdateScheduler::SchedulerTask::TaskType::Policy});
        }

        bool queue_has_policy_and_action()
        {
            return  queue_has_all_tasks(
                    {
                            UpdateScheduler::SchedulerTask::TaskType::Policy,
                            UpdateScheduler::SchedulerTask::TaskType::UpdateNow
                    });
        }

        void wait_policy( const std::string & content)
        {
            while (true)
            {
                auto task = queueTask->pop();
                if (task.taskType == UpdateScheduler::SchedulerTask::TaskType::Policy)
                {
                    if( task.content != content)
                    {
                        std::string info = "Expected policy received has wrong content: ";
                        info += content;
                        throw std::runtime_error(info);
                    }
                }
            }

        }

        bool queue_has_all_tasks( std::vector<UpdateScheduler::SchedulerTask::TaskType> tasks)
        {
            std::vector<bool> values(tasks.size(), false);
            queueTask->pushStop();
            while (true)
            {
                auto task = queueTask->pop();
                if (task.taskType == UpdateScheduler::SchedulerTask::TaskType::Stop)
                {
                    break;
                }
                auto found = std::find(tasks.begin(), tasks.end(), task.taskType );
                if (found != tasks.end())
                {
                    values[std::distance(tasks.begin(), found)] = true;
                }
            }

            return std::all_of(values.begin(), values.end(), [](bool v) { return v; });
        }

    };



    class ManagementAgentIntegrationTests : public ::testing::Test
    {
    public:
        ManagementAgentIntegrationTests() : m_tempDir("/tmp")
        {
            m_tempDir.makeDirs({
                                       {"var/ipc/plugins"},
                                       {"base/mcs/status/cache"},
                                       {"base/mcs/policy"},
                                       {"base/mcs/action"},
                                       {"base/mcs/event"},
                                       {"base/mcs/tmp"},
                                       {"tmp"},
                                       {"base/pluginRegistry"}
                               });
            Common::ApplicationConfiguration::applicationConfiguration().setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_tempDir.dirPath() );
        }


        void TearDown() override
        {
            Common::ApplicationConfiguration::restoreApplicationPathManager();
        }

        std::string updatescheduler()
        {
            std::string jsonString = R"({
"policyAppIds": [
    "ALC"
  ],
  "statusAppIds": [
    "ALC"
  ],
  "pluginName": "updatescheduler",
  "executableFullPath": "base/bin/UpdateScheduler"
})";
            return jsonString;
        }
        Tests::TempDir m_tempDir;

    };

    TEST_F( ManagementAgentIntegrationTests, startWithoutAnyPlugin )
    {
        TestManagementAgent agent;
        ASSERT_EQ(0, agent.runWrapper(std::chrono::seconds(1)));
    }


    TEST_F( ManagementAgentIntegrationTests, noPolicyAvailableToUpdateScheduler )
    {
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        TestManagementAgent agent;
        auto futureRunner = std::async(std::launch::async, [&agent]() {
            return agent.runWrapper(std::chrono::seconds(1));
        });

        FakeSchedulerPlugin plugin;
        ASSERT_EQ(0, futureRunner.get());
        ASSERT_FALSE(plugin.queue_has_policy());
    }

    TEST_F( ManagementAgentIntegrationTests, sendPolicyAndActionToUpdateScheduler )
    {
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        TestManagementAgent agent;
        auto futureRunner = std::async(std::launch::async, [&agent]() {
            return agent.runWrapper(std::chrono::seconds(3));
        });

        FakeSchedulerPlugin plugin;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml",updatePolicyWithProxy);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml",updateAction);
        ASSERT_EQ(0, futureRunner.get());
        ASSERT_TRUE(plugin.queue_has_policy_and_action());
    }


//    TEST_F( ManagementAgentIntegrationTests, scanForPolicyAndActionOnStartup )
//    {
//        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
//        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml",updatePolicyWithProxy);
//        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml",updateAction);
//
//        TestManagementAgent agent;
//        auto futureRunner = std::async(std::launch::async, [&agent]() {
//            return agent.runWrapper(std::chrono::seconds(3));
//        });
//
//        FakeSchedulerPlugin plugin;
//        ASSERT_EQ(0, futureRunner.get());
//        ASSERT_TRUE(plugin.queue_has_policy_and_action());
//    }
//
//
//    TEST_F( ManagementAgentIntegrationTests, trackTheCorrectPolicyForThePlugin )
//    {
//        std::string newPolicy = Common::UtilityImpl::StringUtils::replaceAll(updatePolicyWithProxy, "f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6", "newRevId");
//
//
//        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
//        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml",newPolicy);
//
//        TestManagementAgent agent;
//        auto futureRunner = std::async(std::launch::async, [&agent]() {
//            return agent.runWrapper(std::chrono::seconds(5));
//        });
//
//        std::unique_ptr<FakeSchedulerPlugin> plugin(new FakeSchedulerPlugin);
//
//        plugin->wait_policy(newPolicy);
//
//        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml",updatePolicyWithProxy);
//
//        plugin->wait_policy(updatePolicyWithProxy);
//
//        // plugin register again and receive the correct policy
//        plugin.reset(new FakeSchedulerPlugin);
//
//        plugin->wait_policy( updatePolicyWithProxy);
//
//        ASSERT_EQ(0, futureRunner.get());
//
//
//    }
//



}


