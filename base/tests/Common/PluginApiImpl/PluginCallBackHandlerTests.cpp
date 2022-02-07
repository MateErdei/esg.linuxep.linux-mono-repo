/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "TelemetryHelperImpl/TelemetryHelper.h"
#include "IPluginCallbackApi.h"
#include "ZMQWrapperApi/IContext.h"
#include "ZeroMQWrapper/IReadWrite.h"
#include "PluginApiImpl/PluginCallBackHandler.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"

namespace Common::PluginApiImpl
{
    class PluginCallBackHandlerTests : public LogInitializedTests
    {
    };

    TEST_F(PluginCallBackHandlerTests, pluginCallbackHandlerSavesTelemetryOnExit) //NOLINT
    {
        testing::internal::CaptureStderr();

        std::string pluginName = "Test";

        MockFileSystem* mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
        EXPECT_CALL(*mockFileSystem, isDirectory(_)).WillOnce(Return(true));
        EXPECT_CALL(*mockFileSystem, writeFileAtomically(_, R"({"rootkey":{"array":[{"key1":1}]},"statskey":{}})", "/opt/sophos-spl/tmp"));

        Common::Telemetry::TelemetryHelper& helper = Common::Telemetry::TelemetryHelper::getInstance();
        helper.reset();
        helper.replaceFS(std::unique_ptr<Common::FileSystem::IFileSystem>(mockFileSystem));
        helper.appendObject("array", "key1", 1L);
        std::shared_ptr<Common::PluginApi::IPluginCallbackApi> dummy;

        auto context = Common::ZMQWrapperApi::createContext();
        std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> replier;

        // Creating PluginCallBackHandler in scope so that we force the destructor to be called
        {
            PluginCallBackHandler callbackHandler(pluginName, std::move(replier), std::move(dummy));
        }

        std::string logMessage = testing::internal::GetCapturedStderr();
        EXPECT_THAT(logMessage, ::testing::HasSubstr("Saving plugin telemetry before shutdown"));
    }
}