// Copyright 2023 Sophos Limited. All rights reserved.

#include "telemetry/Telemetry.h"

#include "Common/ConfigFile/ConfigFile.h"
#include "Common/Datatypes/Print.h"
#include "Common/FileSystem/IFileNotFoundException.h"

#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockPlatformUtils.h"
#include "Common/Helpers/MockFileSystem.h"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

using namespace Common::ConfigFile;

namespace
{
    class TestTelemetry : public LogInitializedTests
    {
    public:
        TestTelemetry()
        {
            platform_ = std::make_shared<NiceMock<MockPlatformUtils>>();
        }

        std::shared_ptr<MockPlatformUtils> platform_;
    };
}

using namespace thininstaller::telemetry;

TEST_F(TestTelemetry, noArguments)
{
    std::vector<std::string> args;
    Common::HttpRequests::IHttpRequesterPtr httpRequester;
    Telemetry telemetry{args, httpRequester};
    int ret = telemetry.run(nullptr, *platform_);
    EXPECT_EQ(ret, 1);
}

TEST_F(TestTelemetry, allMissing)
{
    auto mockFileSystem = std::make_unique<MockFileSystem>();
    const auto CONFIG_FILE = "/A.json";
    EXPECT_CALL(*mockFileSystem, readLines(CONFIG_FILE)).WillOnce(Throw(IFileNotFoundException(LOCATION, "Test exception")));
    std::vector<std::string> args{CONFIG_FILE};
    Common::HttpRequests::IHttpRequesterPtr httpRequester;
    Telemetry telemetry{args, httpRequester};
    int ret = telemetry.run(mockFileSystem.get(), *platform_);
    EXPECT_EQ(ret, 0);
}

static std::string expectEmptyConfig(MockFileSystem& fs)
{
    constexpr const auto* CONFIG_FILE = "/config.json";
    ConfigFile::lines_t lines;
    EXPECT_CALL(fs, readLines(CONFIG_FILE)).WillRepeatedly(Return(lines));
    return CONFIG_FILE;
}

TEST_F(TestTelemetry, noTenantId)
{
    auto mockFileSystem = std::make_unique<MockFileSystem>();
    const auto CONFIG_FILE = expectEmptyConfig(*mockFileSystem);
    std::vector<std::string> args{CONFIG_FILE};
    Common::HttpRequests::IHttpRequesterPtr httpRequester; // will fail if it tries to send telemetry
    Telemetry telemetry{args, httpRequester};
    int ret = telemetry.run(mockFileSystem.get(), *platform_);
    EXPECT_EQ(ret, 0);
}