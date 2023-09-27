/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/MockApiBaseServices.h>
#include <Common/Helpers/TempDir.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <modules/pluginimpl/Logger.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <modules/pluginimpl/PluginAdapter.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace Common::FileSystem;

class TestBasicPluginApi: public LogOffInitializedTests{
    public: 
    TestBasicPluginApi(){
        queueTask = std::make_shared<Plugin::QueueTask>(); 
        pluginCallBack = std::make_shared<Plugin::PluginCallback>(queueTask); 
    }   

    ~TestBasicPluginApi(){
        pluginCallBack->onShutdown(); 
        m_runner.get(); 
        pluginAdapter.reset(); 
    }

    void initialiseAdapter( Common::PluginApi::IBaseServiceApi * toOwnedBaseApi)
    {
        pluginAdapter.reset( new Plugin::PluginAdapter(queueTask, std::unique_ptr<Common::PluginApi::IBaseServiceApi>(toOwnedBaseApi), pluginCallBack )); 
        m_runner = std::async(std::launch::async, [this](){
            this->pluginAdapter->mainLoop(); 
        });

    }
    std::shared_ptr<Plugin::QueueTask> queueTask; 
    std::shared_ptr<Plugin::PluginCallback> pluginCallBack; 
    std::unique_ptr<Plugin::PluginAdapter> pluginAdapter; 
    std::future<void> m_runner; 
};

TEST_F(TestBasicPluginApi, shouldBeAbleToHandlePolicyReceived){
    MockApiBaseServices * mock = new MockApiBaseServices(); 
    initialiseAdapter(mock); 
    EXPECT_NO_THROW(pluginCallBack->applyNewPolicy("anythinghere"));     
}

TEST_F(TestBasicPluginApi, shouldRejectInvalidAction){
    MockApiBaseServices * mock = new MockApiBaseServices(); 
    initialiseAdapter(mock); 
    EXPECT_NO_THROW(pluginCallBack->queueActionWithCorrelation("invalidaction", "corrid"));     
}

TEST_F(TestBasicPluginApi, shouldStartNewSessionOnValidAction){

    auto filesystemMock = new ::testing::StrictMock<MockFileSystem>();
    std::string jsonContent =R"({"thumbprint":"1234","url":"dummyurl"})";
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_, _, _)).Times(0);
    EXPECT_CALL(*filesystemMock, isFile(::testing::HasSubstr("mcs.config"))).Times(3).WillRepeatedly(Return(false)); // skip loading endpoint id, device id, tennant id and JWT
    EXPECT_CALL(*filesystemMock, isFile(::testing::HasSubstr("current_proxy"))).WillOnce(Return(false)); // skip loading proxy info
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    MockApiBaseServices * mock = new MockApiBaseServices(); 
    initialiseAdapter(mock); 
    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>1234</thumbprint>
        </action>)";
    EXPECT_NO_THROW(pluginCallBack->queueActionWithCorrelation(content, "corrid"));
}

TEST_F(TestBasicPluginApi, shouldReturnEmptyStatus)
{
    MockApiBaseServices * mock = new MockApiBaseServices();
    initialiseAdapter(mock);
    auto returnedStatus = pluginCallBack->getStatus("live");
    EXPECT_TRUE(returnedStatus.statusXml.empty());
}

TEST_F(TestBasicPluginApi, telemetryShouldProvideVersionInformation)
{
    Tests::TempDir tempdir;         
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempdir.dirPath());

    tempdir.createFile("plugins/liveresponse/VERSION.ini", "PRODUCT_VERSION = 1.2\n");

    MockApiBaseServices * mock = new MockApiBaseServices(); 
    initialiseAdapter(mock); 
    auto returnedTelemetry = pluginCallBack->getTelemetry(); 
    EXPECT_THAT( returnedTelemetry, ::testing::HasSubstr("version"));     
    EXPECT_THAT( returnedTelemetry, ::testing::HasSubstr("1.2"));

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/opt/sophos-spl");

}

TEST_F(TestBasicPluginApi, telemetryShouldNotCrashIfVersionWereNotAvailable)
{
    // Telemetry is initialised during this test in a separate thread so there is a race condition
    // Cannot guarantee 'total-sessions' or 'failed-sessions' will be set by the getTelemetry() call.
    MockApiBaseServices * mock = new MockApiBaseServices(); 
    initialiseAdapter(mock);
    auto returnedTelemetry = pluginCallBack->getTelemetry(); 
    EXPECT_THAT( returnedTelemetry, ::testing::Not(::testing::HasSubstr("version")));     
    EXPECT_FALSE(returnedTelemetry.empty());
}




TEST(TestFileSystemExample, binBashShouldExist) // NOLINT
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    EXPECT_TRUE(ifileSystem->isExecutable("/bin/bash"));
}

TEST(TestLinkerWorks, WorksWithTheLogging) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    LOGINFO("Produce this logging");
}
