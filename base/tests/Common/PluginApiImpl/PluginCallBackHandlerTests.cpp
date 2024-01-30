// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/PluginApiImpl/PluginCallBackHandler.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/ZeroMQWrapper/IReadWrite.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <utility>

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

class TestablePluginCallBackHandler : public Common::PluginApiImpl::PluginCallBackHandler
{
public:
    TestablePluginCallBackHandler(
        const std::string& pluginName,
        std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
        std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback,
        Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY policy =
            Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY::HANDLESHUTDOWN) :
        PluginCallBackHandler(pluginName, std::move(ireadWrite), std::move(pluginCallback), policy)
    {
    }

    std::string exposedGetContentFromPayload(
        Common::PluginProtocol::Commands commandType,
        const std::string& payload) const
    {
        return PluginCallBackHandler::GetContentFromPayload(commandType, payload);
    }
};

TEST_F(PluginCallBackHandlerTests, pluginCallbackHandlerSavesTelemetryOnExit)
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

TEST_F(PluginCallBackHandlerTests, GetContentFromPayloadRetriesOnFailure)
{
    testing::internal::CaptureStderr();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::string payload = "MCS-25_policy.xml";
    std::string fullPath = "/opt/sophos-spl/base/mcs/policy/" + payload;
    std::string policy = "<policy></policy>";
    EXPECT_CALL(*filesystemMock, readFile(fullPath)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed to read"))).WillOnce(Return(policy));
    std::string pluginName = "Test";
    std::shared_ptr<Common::PluginApi::IPluginCallbackApi> dummy;
    std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> replier;
    TestablePluginCallBackHandler testablePluginCallBackHandler(pluginName, std::move(replier), std::move(dummy));
    auto result = testablePluginCallBackHandler.exposedGetContentFromPayload(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, payload);
    ASSERT_EQ(result, policy);
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read MCS file " + payload + ", will retry"));
}

TEST_F(PluginCallBackHandlerTests, GetContentFromPayloadRetriesOnFailureButEventuallyThrows)
{
    testing::internal::CaptureStderr();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::string payload = "MCS-25_policy.xml";
    std::string fullPath = "/opt/sophos-spl/base/mcs/policy/" + payload;
    EXPECT_CALL(*filesystemMock, readFile(fullPath)).WillRepeatedly(Throw(Common::FileSystem::IFileSystemException("Failed to read")));
    std::string pluginName = "Test";
    std::shared_ptr<Common::PluginApi::IPluginCallbackApi> dummy;
    std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> replier;
    TestablePluginCallBackHandler testablePluginCallBackHandler(pluginName, std::move(replier), std::move(dummy));
    ASSERT_THROW(testablePluginCallBackHandler.exposedGetContentFromPayload(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, payload), std::exception);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read MCS file " + payload + ", will retry"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read MCS file " + payload + ". Exception: Failed to read"));
}