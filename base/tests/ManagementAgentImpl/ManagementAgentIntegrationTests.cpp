// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystemImpl/FilePermissionsImpl.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/PluginApi/IPluginResourceManagement.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h"
#include "ManagementAgent/PluginCommunicationImpl/PluginManager.h"
#include "UpdateScheduler/SchedulerTaskQueue.h"
#include "UpdateSchedulerImpl/SchedulerPluginCallback.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/TempDir.h"
#include "tests/Common/Helpers/TestExecutionSynchronizer.h"
#include "tests/Common/TaskQueueImpl/FakeQueue.h"
#include "tests/ManagementAgent/MockPluginManager/MockPluginManager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>
#include <thread>

namespace
{
    std::string updatePolicyWithProxy{ R"sophos(<?xml version="1.0"?>
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
)sophos" };

    std::string updateAction{ R"sophos(<?xml version='1.0'?>
<action type="sophos.mgt.action.ALCForceUpdate"/>)sophos" };

    class TestManagementAgent : public ManagementAgent::ManagementAgentImpl::ManagementAgentMain
    {
    public:
        TestManagementAgent() = default;

        int runWrapper(Tests::TestExecutionSynchronizer& synchronizer, std::chrono::milliseconds duration)
        {
            auto pluginManager = std::make_unique<ManagementAgent::PluginCommunicationImpl::PluginManager>(
                        Common::ZMQWrapperApi::createContext());

            initialise(std::move(pluginManager));
            auto futureRunner = std::async(std::launch::async, [this]() { return run(false); });
            synchronizer.waitfor(duration);
            test_request_stop();
            return futureRunner.get();
        }

        Common::TaskQueue::ITaskQueueSharedPtr getTaskQueue() { return m_taskQueue; }

        bool updateOngoingWithGracePeriod(unsigned int gracePeriodSeconds)
        {
            auto pluginManager = std::make_unique<ManagementAgent::PluginCommunicationImpl::PluginManager>(
                Common::ZMQWrapperApi::createContext());
            auto now = std::chrono::steady_clock::now();
            return pluginManager->updateOngoingWithGracePeriod(gracePeriodSeconds, now);
        }
    };

    class FakeSchedulerPlugin
    {
    public:
        FakeSchedulerPlugin() :
            m_resourceManagement(Common::PluginApi::createPluginResourceManagement()),
            m_queueTask(std::make_shared<UpdateScheduler::SchedulerTaskQueue>()),
            m_sharedPluginCallBack(std::make_shared<UpdateSchedulerImpl::SchedulerPluginCallback>(m_queueTask)),
            m_baseService(m_resourceManagement->createPluginAPI("updatescheduler", m_sharedPluginCallBack))
        {
        }

        std::unique_ptr<Common::PluginApi::IPluginResourceManagement> m_resourceManagement;
        std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_queueTask;
        std::shared_ptr<UpdateSchedulerImpl::SchedulerPluginCallback> m_sharedPluginCallBack;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        bool queue_has_policy() { return queue_has_all_tasks({ UpdateScheduler::SchedulerTask::TaskType::Policy }); }

        bool queue_has_policy_and_action()
        {
            return queue_has_all_tasks({ UpdateScheduler::SchedulerTask::TaskType::Policy,
                                         UpdateScheduler::SchedulerTask::TaskType::UpdateNow });
        }
        void stop_test() { m_queueTask->pushStop(); }

        void wait_policy(const std::string& content)
        {
            while (true)
            {
                UpdateScheduler::SchedulerTask task;
                m_queueTask->pop(task, 1);
                if (task.taskType == UpdateScheduler::SchedulerTask::TaskType::Policy)
                {
                    if (task.content != content)
                    {
                        std::stringstream info;
                        info << "Expected policy received has wrong content: " << content.substr(0, 400)
                             << "\nExpected content: " << task.content.substr(0, 400);
                        throw std::runtime_error(info.str());
                    }
                    else
                    {
                        return;
                    }
                }
                std::stringstream ss;
                ss << "Unexpected task received: " << static_cast<int>(task.taskType);
                throw std::runtime_error(ss.str());
            }
        }
        void wait_action()
        {
            while (true)
            {
                UpdateScheduler::SchedulerTask task;
                m_queueTask->pop(task, 1);
                if (task.taskType == UpdateScheduler::SchedulerTask::TaskType::UpdateNow)
                {
                    return;
                }
                else
                {
                    throw std::runtime_error("Unexpected task received ");
                }
            }
        }
        void clearQueue()
        {
            m_queueTask->pushStop();
            UpdateScheduler::SchedulerTask task;
            while (m_queueTask->pop(task, 1))
            {
                if (task.taskType == UpdateScheduler::SchedulerTask::TaskType::Stop)
                {
                    break;
                }
            }
        }

        bool queue_has_all_tasks(std::vector<UpdateScheduler::SchedulerTask::TaskType> tasks)
        {
            std::vector<bool> values(tasks.size(), false);
            m_queueTask->pushStop();
            while (true)
            {
                UpdateScheduler::SchedulerTask task;
                m_queueTask->pop(task, 1);
                if (task.taskType == UpdateScheduler::SchedulerTask::TaskType::Stop)
                {
                    break;
                }
                auto found = std::find(tasks.begin(), tasks.end(), task.taskType);
                if (found != tasks.end())
                {
                    values[std::distance(tasks.begin(), found)] = true;
                }
            }

            return std::all_of(values.begin(), values.end(), [](bool v) { return v; });
        }
    };

    class FilePermisssionsImplNoChmodChown : public Common::FileSystem::FilePermissionsImpl
    {
    public:
        // Dummy function override to allow the rest of filesystem to be used
        void chmod(const Path& /*path*/, __mode_t /*mode*/) const override {}
        void chown(const Path& /*path*/, const std::string& /*user*/, const std::string& /*group*/) const override {}
        unsigned int getUserId(const std::string& /*user*/) const override {return 1;}
    };

    class ManagementAgentIntegrationTests : public ::testing::Test
    {
    public:
        ManagementAgentIntegrationTests() : m_tempDir("/tmp", "mait")
        {
            if (m_tempDir.exists("base/mcs/action/ALC_action_1.xml"))
            {
                throw std::runtime_error("Directory already existed. Can not run test. ");
            }
            m_tempDir.makeDirs({ { "var/ipc/plugins" },
                                 { "base/mcs/status/cache" },
                                 { "base/mcs/policy" },
                                 { "base/mcs/internal_policy" },
                                 { "base/mcs/action" },
                                 { "base/mcs/event" },
                                 { "base/mcs/tmp" },
                                 { "tmp" },
                                 { "base/pluginRegistry" } });

            Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, m_tempDir.dirPath());

            std::unique_ptr<FilePermisssionsImplNoChmodChown> iFilePermissionsPtr =
                std::unique_ptr<FilePermisssionsImplNoChmodChown>(new FilePermisssionsImplNoChmodChown);
            Tests::replaceFilePermissions(std::move(iFilePermissionsPtr));
        }

        void TearDown() override { Common::ApplicationConfiguration::restoreApplicationPathManager(); }

        std::string updatescheduler()
        {
            std::string jsonString = R"({
"policyAppIds": [
    "ALC"
  ],
  "actionAppIds": [
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
        Common::Logging::ConsoleLoggingSetup m_loggerSetup;
    };

    // cppcheck-suppress syntaxError
    TEST_F(ManagementAgentIntegrationTests, managementAgentWihoutAnyPluginShouldRunNormaly) // NOLINT
    {
        TestManagementAgent agent;
        Tests::TestExecutionSynchronizer synchronizer;
        ASSERT_EQ(0, agent.runWrapper(synchronizer, std::chrono::milliseconds(300)));
    }

    TEST_F( // NOLINT
        ManagementAgentIntegrationTests,
        ifNoPolicyIsAvailableToAPluginManagementAgentShouldNotSendAnyPolicy)
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

    TEST_F( // NOLINT
        ManagementAgentIntegrationTests,
        ifALCPolicyOrActionIsDroppedByMCSRouterManagementAgentShouldSendThemToUpdateScheduler)
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
            if (!pluginSynchronizer.waitfor(std::chrono::seconds(2)))
            {
                // if timeout
                plugin.stop_test();
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml", updatePolicyWithProxy);
        plugin.wait_policy(updatePolicyWithProxy);
        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml", updateAction);
        plugin.wait_action();
        synchronizer.notify();
        pluginSynchronizer.notify();
        ASSERT_EQ(0, futureRunner.get());
    }

    TEST_F( // NOLINT
        ManagementAgentIntegrationTests,
        onStartupManagementAgentShouldScanForPolicyOrActionsAndSendThemToPlugins)
    {
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml", updatePolicyWithProxy);
        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml", updateAction);
        Tests::TestExecutionSynchronizer synchronizer;
        TestManagementAgent agent;
        auto futureRunner = std::async(std::launch::async, [&agent, &synchronizer]() {
            return agent.runWrapper(synchronizer, std::chrono::milliseconds(3000));
        });

        FakeSchedulerPlugin plugin;
        Tests::TestExecutionSynchronizer pluginSynchronizer;
        auto timeoutProtect = std::async(std::launch::async, [&plugin, &pluginSynchronizer]() {
            if (!pluginSynchronizer.waitfor(std::chrono::seconds(2)))
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

    TEST_F( // NOLINT
        ManagementAgentIntegrationTests,
        afterCrashManagementAgentShouldSendPendingPoliciesAndActionsToPlugins)
    {
        std::string newPolicy = Common::UtilityImpl::StringUtils::replaceAll(
            updatePolicyWithProxy, "f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6", "newRevId");
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        std::unique_ptr<TestManagementAgent> agent(new TestManagementAgent());
        Tests::TestExecutionSynchronizer synchronizer;
        auto futureRunner = std::async(std::launch::async, [&agent, &synchronizer]() {
            return agent->runWrapper(synchronizer, std::chrono::milliseconds(3000));
        });

        FakeSchedulerPlugin plugin;
        Tests::TestExecutionSynchronizer pluginSynchronizer;
        auto timeoutProtect = std::async(std::launch::async, [&plugin, &pluginSynchronizer]() {
            if (!pluginSynchronizer.waitfor(std::chrono::seconds(2)))
            {
                // if timeout
                plugin.stop_test();
            }
        });

        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml", newPolicy);
        plugin.wait_policy(newPolicy);

        synchronizer.notify();
        ASSERT_EQ(0, futureRunner.get());
        plugin.clearQueue();
        agent.reset();
        // while management agent is not running
        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml", updateAction);
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml", updatePolicyWithProxy);
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

    // disabled only because the timings depend on the socket timeout and need the logging to verify.
    // Keeping it can be run manually.
    TEST_F(ManagementAgentIntegrationTests, DISABLED_ifPluginIsNotAvailableManagementAgentShouldNotCrash) // NOLINT
    {
        testing::internal::CaptureStderr();

        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        TestManagementAgent agent;
        Tests::TestExecutionSynchronizer synchronizer;
        auto futureRunner = std::async(std::launch::async, [&agent, &synchronizer]() {
            return agent.runWrapper(synchronizer, std::chrono::milliseconds(2000));
        });

        std::unique_ptr<FakeSchedulerPlugin> plugin(new FakeSchedulerPlugin());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml", updatePolicyWithProxy);
        plugin->wait_policy(updatePolicyWithProxy);

        // simulate plugin crashed.
        plugin.reset();

        // drop an action and management agent should not crash
        m_tempDir.createFileAtomically("base/mcs/action/ALC_action_1.xml", updateAction);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        synchronizer.notify();

        ASSERT_EQ(0, futureRunner.get());
        std::string errStd = testing::internal::GetCapturedStderr();
        ASSERT_THAT(errStd, ::testing::HasSubstr("Failure on sending action to updatescheduler"));
    }

    TEST_F(ManagementAgentIntegrationTests, onlyPolicyALCShouldBeDeliveredToUpdateScheduler) // NOLINT
    {
        m_tempDir.createFile("base/pluginRegistry/updatescheduler.json", updatescheduler());
        std::string notWantedPolicy = "notWantedPolicy";

        TestManagementAgent agent;
        Tests::TestExecutionSynchronizer synchronizer;
        auto futureRunner = std::async(std::launch::async, [&agent, &synchronizer]() {
            return agent.runWrapper(synchronizer, std::chrono::milliseconds(3000));
        });

        FakeSchedulerPlugin plugin;
        Tests::TestExecutionSynchronizer pluginSynchronizer;
        auto timeoutProtect = std::async(std::launch::async, [&plugin, &pluginSynchronizer]() {
            if (!pluginSynchronizer.waitfor(std::chrono::seconds(2)))
            {
                // if timeout
                plugin.stop_test();
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_tempDir.createFileAtomically("base/mcs/policy/NOALC-1_policy.xml", notWantedPolicy);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_tempDir.createFileAtomically("base/mcs/policy/ALC-1_policy.xml", updatePolicyWithProxy);
        plugin.wait_policy(updatePolicyWithProxy);
        synchronizer.notify();
        pluginSynchronizer.notify();
        ASSERT_EQ(0, futureRunner.get());
    }
} // namespace
