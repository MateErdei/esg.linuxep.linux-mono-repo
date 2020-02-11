/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include <pluginimpl/PluginAdapter.h>
#include <pluginimpl/PluginAdapter.cpp>
#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <fstream>
#include <Common/Logging/ConsoleLoggingSetup.h>

using namespace Plugin;

using namespace ::testing;

namespace fs = sophos_filesystem;

#define BASE "/tmp/TestPluginAdapter"

void setupFakeSophosThreatDetectorConfig()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", BASE);
    fs::path f = BASE;
    f /= "sbin";
    fs::create_directories(f);
    f /= "sophos_threat_detector_launcher";
    std::ofstream ost(f);
    ost.close();
}

TEST(TestPluginAdapter, testConstruction) //NOLINT
{
    std::shared_ptr<Plugin::QueueTask> queueTask;
    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;
    std::shared_ptr<Plugin::PluginCallback> callback;

    setupFakeSophosThreatDetectorConfig();

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), callback);
}

TEST(TestPluginAdapter, testProcessPolicy) //NOLINT
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
    testing::internal::CaptureStderr();
    std::shared_ptr<QueueTask> queueTask = std::make_shared<QueueTask>();
    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;
    std::shared_ptr<Plugin::PluginCallback> callback;

    setupFakeSophosThreatDetectorConfig();

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), callback);

    std::string policyXml =
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
</config>
)MULTILINE";

    Task actionTask = {Task::TaskType::Policy,
                       policyXml};
    Task stopTask = {Task::TaskType::Stop, ""};

    queueTask->push(actionTask);
    queueTask->push(stopTask);

    pluginAdapter.mainLoop();
    std::string expectedLog = "Process policy: ";
    expectedLog.append(policyXml);
    std::string logs = testing::internal::GetCapturedStderr();

    EXPECT_NE(logs.find(expectedLog), std::string::npos);
    EXPECT_NE(logs.find("Updating scheduled scan configuration"), std::string::npos);
}


TEST(TestPluginAdapter, testProcessAction) //NOLINT
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
    testing::internal::CaptureStderr();
    std::shared_ptr<QueueTask> queueTask = std::make_shared<QueueTask>();
    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;
    std::shared_ptr<Plugin::PluginCallback> callback;

    setupFakeSophosThreatDetectorConfig();

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), callback);

    std::string actionXml =
            R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>)";

    Task actionTask = {Task::TaskType::Action,
                       actionXml};
    Task stopTask = {Task::TaskType::Stop, ""};

    queueTask->push(actionTask);
    queueTask->push(stopTask);

    pluginAdapter.mainLoop();
    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);
    std::string logs = testing::internal::GetCapturedStderr();

    EXPECT_NE(logs.find(expectedLog), std::string::npos);
    EXPECT_NE(logs.find("Starting Scan Now scan"), std::string::npos);
}

TEST(TestPluginAdapter, testProcessActionMalformed) //NOLINT
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
    testing::internal::CaptureStderr();
    std::shared_ptr<QueueTask> queueTask = std::make_shared<QueueTask>();
    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;
    std::shared_ptr<Plugin::PluginCallback> callback;

    setupFakeSophosThreatDetectorConfig();

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), callback);

    std::string actionXml =
            R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="NONE" id="" subtype="MALFORMED" replyRequired="0"/>)";

    Task actionTask = {Task::TaskType::Action,
                       actionXml};
    Task stopTask = {Task::TaskType::Stop, ""};

    queueTask->push(actionTask);
    queueTask->push(stopTask);

    pluginAdapter.mainLoop();
    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);
    std::string logs = testing::internal::GetCapturedStderr();

    EXPECT_NE(logs.find(expectedLog), std::string::npos);
    EXPECT_EQ(logs.find("Starting Scan Now scan"), std::string::npos);
}
