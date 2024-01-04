// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "telemetry/Telemetry.h"

#include "Common/ConfigFile/ConfigFile.h"
#include "Common/Datatypes/Print.h"
#include "Common/FileSystem/IFileNotFoundException.h"

#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockHttpRequester.h"
#include "Common/Helpers/MockPlatformUtils.h"

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

static std::string expectFile(MockFileSystem& fs, const std::string& path, const ConfigFile::lines_t& lines)
{
    EXPECT_CALL(fs, readLines(path)).WillRepeatedly(Return(lines));
    EXPECT_CALL(fs, readFile(path)).WillRepeatedly(Return(path));
    return path;
}

static std::string expectEmptyConfig(MockFileSystem& fs)
{
    constexpr const auto* CONFIG_FILE = "/config.json";
    return expectFile(fs, CONFIG_FILE, {});
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

TEST_F(TestTelemetry, proxyFailureFallsbackToDirect)
{
    using namespace Common::HttpRequests;
    auto mockFileSystem = std::make_unique<MockFileSystem>();
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();

    std::vector<Common::HttpRequests::RequestConfig> requests;
    EXPECT_CALL(*httpRequester, put(_)).Times(2).WillRepeatedly([&requests](Common::HttpRequests::RequestConfig request){
        requests.push_back(request);
        Common::HttpRequests::Response response;
        if (request.proxy)
        {
            response.status = 500;
            response.errorCode = ResponseErrorCode::FAILED;
        }
        else
        {
            response.status = HTTP_STATUS_OK;
            response.errorCode = ResponseErrorCode::OK;
        }
        return response;
    });
    const auto CONFIG_FILE = expectFile(*mockFileSystem, "/config.ini", {"TENANT_ID=ABC", "URL=https://mcs.localhost/"});
    const auto PROXY_FILE = expectFile(*mockFileSystem, "/registration_comms_check.ini", {"proxyOrMessageRelayURL=http://proxy:8000"});
    std::vector<std::string> args{CONFIG_FILE, PROXY_FILE};
    Telemetry telemetry{args, httpRequester};
    int ret = telemetry.run(mockFileSystem.get(), *platform_);
    EXPECT_EQ(ret, 0);
    ASSERT_EQ(requests.size(), 2);
    auto request1 = requests.at(0);
    ASSERT_TRUE(request1.proxy);
    EXPECT_EQ(request1.proxy.value(), "http://proxy:8000");
    auto request2 = requests.at(1);
    EXPECT_FALSE(request2.proxy);

}
