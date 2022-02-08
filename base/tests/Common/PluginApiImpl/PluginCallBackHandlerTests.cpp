/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IPluginCallbackApi.h"
#include "FileSystemImpl/FileSystemImpl.h"
#include "PluginApiImpl/PluginCallBackHandler.h"
#include "TelemetryHelperImpl/TelemetryHelper.h"
#include "ZeroMQWrapper/IReadWrite.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"

class ScopeInsertFSMock{
    Common::Telemetry::TelemetryHelper & m_helper;
public:
    explicit ScopeInsertFSMock( IFileSystem * mock, Common::Telemetry::TelemetryHelper & helper):
        m_helper(helper)
    {
        m_helper.replaceFS(std::unique_ptr<Common::FileSystem::IFileSystem>(mock));
    }
    ~ScopeInsertFSMock()
    {
        m_helper.replaceFS(std::unique_ptr<Common::FileSystem::IFileSystem>(new Common::FileSystem::FileSystemImpl()));
    }
};

class PluginCallBackHandlerTests : public LogInitializedTests
{
};

TEST_F(PluginCallBackHandlerTests, pluginCallbackHandlerSavesTelemetryOnExit) // NOLINT
{
    testing::internal::CaptureStderr();

    MockFileSystem* mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, R"({"rootkey":{"array":[{"key1":1}]},"statskey":{}})", "/opt/sophos-spl/tmp"));

    Common::Telemetry::TelemetryHelper& helper = Common::Telemetry::TelemetryHelper::getInstance();
    ScopeInsertFSMock s(mockFileSystem, helper);
    helper.reset();
    helper.appendObject("array", "key1", 1L);

    std::string pluginName = "Test";
    std::shared_ptr<Common::PluginApi::IPluginCallbackApi> dummy;
    std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> replier;

    // Creating PluginCallBackHandler in scope so that we force the destructor to be called
    {
        Common::PluginApiImpl::PluginCallBackHandler callbackHandler(pluginName, std::move(replier), std::move(dummy));
    }

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Saving plugin telemetry before shutdown"));
}