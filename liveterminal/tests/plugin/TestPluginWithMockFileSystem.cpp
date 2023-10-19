/******************************************************************************************************

Copyright 2018-2022, Sophos Limited.  All rights reserved.

 ******************************************************************************************************/

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockApiBaseServices.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"
#include "base/tests/Common/Helpers/TempDir.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockApiBaseServices.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/TempDir.h"
#endif

#include "pluginimpl/Logger.h"
#include "pluginimpl/PluginAdapter.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace Common::FileSystem;

class TestBasicPluginApiWithMockFileSystem : public LogOffInitializedTests
{
public:
    TestBasicPluginApiWithMockFileSystem()
    {
        queueTask = std::make_shared<Plugin::QueueTask>();
        pluginCallBack = std::make_shared<Plugin::PluginCallback>(queueTask);
    }

    ~TestBasicPluginApiWithMockFileSystem()
    {
        pluginCallBack->onShutdown();
        m_runner.get();
        pluginAdapter.reset();
    }

    void initialiseAdapter(Common::PluginApi::IBaseServiceApi* toOwnedBaseApi)
    {
        pluginAdapter.reset(new Plugin::PluginAdapter(
            queueTask, std::unique_ptr<Common::PluginApi::IBaseServiceApi>(toOwnedBaseApi), pluginCallBack));
        m_runner = std::async(std::launch::async, [this]() { this->pluginAdapter->mainLoop(); });
    }
    std::shared_ptr<Plugin::QueueTask> queueTask;
    std::shared_ptr<Plugin::PluginCallback> pluginCallBack;
    std::unique_ptr<Plugin::PluginAdapter> pluginAdapter;
    std::future<void> m_runner;
};

// This test was originally in TestPlugin.cpp but the mock file system would interfere with other tests
// Using a ScopedFileSystem would get other tests to pass but then this test would fail
// Easiest solution found is to just move this test to a different file
TEST_F(TestBasicPluginApiWithMockFileSystem, shouldStartNewSessionOnValidAction)
{
    auto filesystemMock = new ::testing::StrictMock<MockFileSystem>();
    std::string jsonContent = R"({"thumbprint":"1234","url":"dummyurl"})";
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_, _, _)).Times(0);
    EXPECT_CALL(*filesystemMock, isFile(::testing::HasSubstr("mcs.config")))
        .Times(3)
        .WillRepeatedly(Return(false)); // skip loading endpoint id, device id, tennant id and JWT
    EXPECT_CALL(*filesystemMock, isFile(::testing::HasSubstr("current_proxy")))
        .WillOnce(Return(false)); // skip loading proxy info
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    MockApiBaseServices* mock = new MockApiBaseServices();
    initialiseAdapter(mock);
    std::string content = R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>1234</thumbprint>
        </action>)";

    EXPECT_NO_THROW(pluginCallBack->queueActionWithCorrelation(content, "corrid"));
}