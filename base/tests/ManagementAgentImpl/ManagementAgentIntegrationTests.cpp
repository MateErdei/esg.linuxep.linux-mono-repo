/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <tests/ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>
#include <tests/Common/TestHelpers/TestExecutionSynchronizer.h>
#include <tests/Common/TestHelpers/TempDir.h>
#include <ManagementAgent/LoggerImpl/LoggingSetup.h>
#include <ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <UpdateSchedulerImpl/LoggingSetup.h>
#include <UpdateSchedulerImpl/SchedulerPluginCallback.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <future>

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


        int runWrapper(Tests::TestExecutionSynchronizer & synchronizer, std::chrono::milliseconds duration)
        {
            ;

            std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager> pluginManager = std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager>(
                    new ManagementAgent::PluginCommunicationImpl::PluginManager());

            initialise(*pluginManager);
            auto futureRunner = std::async(std::launch::async, [this]() {
                return run();
            });
            synchronizer.waitfor(duration);
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
        void stop_test()
        {
            queueTask->pushStop();
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
                        std::stringstream info ;
                        info << "Expected policy received has wrong content: "
                            << content.substr(0, 400)
                            <<  "\nExpected content: "
                            << task.content.substr(0,400);
                        throw std::runtime_error(info.str());
                    }
                    else
                    {
                        return;
                    }
                }
                throw std::runtime_error("Unexpected task received. ");
            }

        }
        void wait_action()
        {
            while (true)
            {
                auto task = queueTask->pop();
                if (task.taskType == UpdateScheduler::SchedulerTask::TaskType::UpdateNow)
                {
                    return ;
                }
                else
                {
                    throw std::runtime_error("Unexpected task received ");
                }
            }

        }
        void clearQueue()
        {
            queueTask->pushStop();
            while (queueTask->pop().taskType != UpdateScheduler::SchedulerTask::TaskType::Stop)
            {

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
        ManagementAgentIntegrationTests() : m_tempDir("/tmp", "mait"), loggerSetup(1)
        {
            if( m_tempDir.exists("base/mcs/action/ALC_action_1.xml"))
            {
                throw std::runtime_error("Directory already existed. Can not run test. ");
            }
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
        ManagementAgent::LoggerImpl::LoggingSetup loggerSetup;

    };

    TEST_F( ManagementAgentIntegrationTests, managementAgentWihoutAnyPluginShouldRunNormaly )
    {
        TestManagementAgent agent;
        Tests::TestExecutionSynchronizer synchronizer;
        ASSERT_EQ(0, agent.runWrapper(synchronizer, std::chrono::milliseconds(300)));
    }


    TEST_F( ManagementAgentIntegrationTests, ifNoPolicyIsAvailableToAPluginManagementAgentShouldNotSendAnyPolicy )
    {
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());

        TestManagementAgent agent;
        auto futureRunner = std::async(std::launch::async, [&agent]() {
            Tests::TestExecutionSynchronizer synchronizer;
            return agent.runWrapper(synchronizer, std::chrono::milliseconds(300));
        });

        FakeSchedulerPlugin plugin;
        ASSERT_EQ(0, futureRunner.get());
        ASSERT_FALSE(plugin.queue_has_policy());
    }

    TEST_F( ManagementAgentIntegrationTests, ifALCPolicyOrActionIsDroppedByMCSRouterManagementAgentShouldSendThemToUpdateScheduler )
    {
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        TestManagementAgent agent;
        Tests::TestExecutionSynchronizer synchronizer;
        auto futureRunner = std::async(std::launch::async, [&agent, &synchronizer]() {
            return agent.runWrapper(synchronizer, std::chrono::milliseconds(3000));
        });

        FakeSchedulerPlugin plugin;
        Tests::TestExecutionSynchronizer pluginSynchronizer;
        auto timeoutProtect = std::async(std::launch::async, [&plugin, &pluginSynchronizer]() {
            if(!pluginSynchronizer.waitfor(std::chrono::seconds(2)))
            {
                // if timeout
                plugin.stop_test();
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml",updatePolicyWithProxy);
        plugin.wait_policy(updatePolicyWithProxy);
        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml",updateAction);
        plugin.wait_action();
        synchronizer.notify();
        pluginSynchronizer.notify();
        ASSERT_EQ(0, futureRunner.get());
    }


    TEST_F( ManagementAgentIntegrationTests, onStartupManagementAgentShouldScanForPolicyOrActionsAndSendThemToPlugins )
    {
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml",updatePolicyWithProxy);
        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml",updateAction);
        Tests::TestExecutionSynchronizer synchronizer;
        TestManagementAgent agent;
        auto futureRunner = std::async(std::launch::async, [&agent, &synchronizer]() {
            return agent.runWrapper(synchronizer, std::chrono::milliseconds(3000));
        });

        FakeSchedulerPlugin plugin;
        Tests::TestExecutionSynchronizer pluginSynchronizer;
        auto timeoutProtect = std::async(std::launch::async, [&plugin, &pluginSynchronizer]() {
            if(!pluginSynchronizer.waitfor(std::chrono::seconds(2)))
            {
                // if timeout
                plugin.stop_test();
            }
        });

        plugin.wait_policy(updatePolicyWithProxy);
        plugin.wait_action();
        pluginSynchronizer.notify();
        synchronizer.notify();

        ASSERT_EQ(0, futureRunner.get());
    }


    TEST_F( ManagementAgentIntegrationTests, afterCrashManagementAgentShouldSendPendingPoliciesAndActionsToPlugins )
    {
        std::string newPolicy = Common::UtilityImpl::StringUtils::replaceAll(updatePolicyWithProxy, "f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6", "newRevId");
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        std::unique_ptr<TestManagementAgent> agent( new TestManagementAgent());
                Tests::TestExecutionSynchronizer synchronizer;
        auto futureRunner = std::async(std::launch::async, [&agent, &synchronizer]() {
            return agent->runWrapper(synchronizer, std::chrono::milliseconds(3000));
        });

        FakeSchedulerPlugin plugin;
        Tests::TestExecutionSynchronizer pluginSynchronizer;
        auto timeoutProtect = std::async(std::launch::async, [&plugin, &pluginSynchronizer]() {
            if(!pluginSynchronizer.waitfor(std::chrono::seconds(2)))
            {
                // if timeout
                plugin.stop_test();
            }
        });

        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml",newPolicy);
        plugin.wait_policy(newPolicy);

        synchronizer.notify();
        ASSERT_EQ(0, futureRunner.get());
        plugin.clearQueue();
        agent.reset();
        // while management agent is not running
        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml",updateAction);
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml",updatePolicyWithProxy);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        agent.reset(new TestManagementAgent());
        Tests::TestExecutionSynchronizer newAgentsynchronizer;
        auto secondFutureRunner = std::async(std::launch::async, [&agent, &newAgentsynchronizer]() {
            return agent->runWrapper(newAgentsynchronizer, std::chrono::milliseconds(3000));
        });
        plugin.wait_policy(updatePolicyWithProxy);
        plugin.wait_action();
        newAgentsynchronizer.notify();
        pluginSynchronizer.notify();
        ASSERT_EQ(0, secondFutureRunner.get());

    }




}


