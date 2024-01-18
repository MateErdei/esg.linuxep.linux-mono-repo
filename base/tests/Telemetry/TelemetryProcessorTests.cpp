// Copyright 2019-2023 Sophos Limited. All rights reserved.

/**
 * Component tests for Telemetry Executable
 */

#include "DerivedTelemetryProcessor.h"
#include "MockTelemetryProvider.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Telemetry/TelemetryImpl/TelemetryProcessor.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockHttpRequester.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::StrictMock;

class TelemetryProcessorTest : public LogInitializedTests
{
public:
    const char* m_defaultServer = "t1.sophosupd.com";
    const std::string m_jsonFilePath = "/opt/sophos-spl/base/telemetry/var/telemetry.json";
    const std::string m_telemetryStatusFilePath = "/opt/sophos-spl/base/telemetry/var/telemetry-status.json";

    MockFileSystem* m_mockFileSystem = nullptr;
    MockFilePermissions* m_mockFilePermissions = nullptr;
    std::shared_ptr<Common::TelemetryConfigImpl::Config> m_config;
    std::shared_ptr<MockHTTPRequester> m_httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Tests::ScopedReplaceFileSystem m_replacer; 

    void SetUp() override
    {
        m_mockFileSystem =new StrictMock<MockFileSystem>();
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_mockFileSystem));

        std::unique_ptr<MockFilePermissions> mockfilePermissions(new StrictMock<MockFilePermissions>());
        m_mockFilePermissions = mockfilePermissions.get();
        Tests::replaceFilePermissions(std::move(mockfilePermissions));

        m_config = std::make_shared<Common::TelemetryConfigImpl::Config>();
        m_config->setMaxJsonSize(1000);
    }

    void TearDown() override { Tests::restoreFilePermissions(); }
};

TEST_F(TelemetryProcessorTest, telemetryProcessorNoProviders)
{
    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    DerivedTelemetryProcessor telemetryProcessor(m_config, std::move(m_httpRequester), telemetryProviders);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ("{}", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorOneProvider)
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;

    telemetryProviders.emplace_back(mockTelemetryProvider);

    DerivedTelemetryProcessor telemetryProcessor(m_config, std::move(m_httpRequester), telemetryProviders);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock":{"key":1}})", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorTwoProviders)
{
    auto mockTelemetryProvider1 = std::make_shared<MockTelemetryProvider>();
    auto mockTelemetryProvider2 = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider1, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider1, getName()).WillOnce(Return("Mock1"));

    EXPECT_CALL(*mockTelemetryProvider2, getTelemetry()).WillOnce(Return(R"({"key":2})"));
    EXPECT_CALL(*mockTelemetryProvider2, getName()).WillOnce(Return("Mock2"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;

    telemetryProviders.emplace_back(mockTelemetryProvider1);
    telemetryProviders.emplace_back(mockTelemetryProvider2);

    DerivedTelemetryProcessor telemetryProcessor(m_config, std::move(m_httpRequester), telemetryProviders);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock1":{"key":1},"Mock2":{"key":2}})", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorThreeProvidersOneThrows)
{
    auto mockTelemetryProvider1 = std::make_shared<MockTelemetryProvider>();
    auto mockTelemetryProvider2 = std::make_shared<MockTelemetryProvider>();
    auto mockTelemetryProvider3 = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider1, getName()).WillOnce(Return("Mock1"));
    EXPECT_CALL(*mockTelemetryProvider1, getTelemetry()).WillOnce(Return(R"({"key":1})"));

    EXPECT_CALL(*mockTelemetryProvider2, getName()).WillOnce(Return("Mock2"));
    EXPECT_CALL(*mockTelemetryProvider2, getTelemetry()).WillOnce(Throw(std::runtime_error("badProvider")));

    EXPECT_CALL(*mockTelemetryProvider3, getName()).WillOnce(Return("Mock3"));
    EXPECT_CALL(*mockTelemetryProvider3, getTelemetry()).WillOnce(Return(R"({"key":3})"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;

    telemetryProviders.emplace_back(mockTelemetryProvider1);
    telemetryProviders.emplace_back(mockTelemetryProvider2);
    telemetryProviders.emplace_back(mockTelemetryProvider3);

    DerivedTelemetryProcessor telemetryProcessor(m_config, std::move(m_httpRequester), telemetryProviders);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock1":{"key":1},"Mock2":{"health":1},"Mock3":{"key":3}})", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorWritesJsonToFile)
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    Common::HttpRequests::Response response;
    response.status = 200;
    response.errorCode = Common::HttpRequests::ResponseErrorCode::OK;

    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, R"({"Mock":{"key":1}})")).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(HasSubstr("current_proxy"))).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));
    EXPECT_CALL(*m_httpRequester, put(_)).WillOnce(Return(response));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    auto config = std::make_shared<Common::TelemetryConfigImpl::Config>();
    config->setResourceRoot("PROD");
    config->setServer(m_defaultServer);
    config->setMaxJsonSize(1000);

    DerivedTelemetryProcessor telemetryProcessor(config, std::move(m_httpRequester), telemetryProviders);
    telemetryProcessor.Run();
}

TEST_F(TelemetryProcessorTest, telemetryProcessorSucessFullyUsesProxy)
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    Common::HttpRequests::Response response;
    response.status = 200;
    response.errorCode = Common::HttpRequests::ResponseErrorCode::OK;

    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, R"({"Mock":{"key":1}})")).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(HasSubstr("current_proxy"))).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));
    EXPECT_CALL(*m_httpRequester, put(_)).WillOnce(Return(response));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    auto config = std::make_shared<Common::TelemetryConfigImpl::Config>();
    config->setResourceRoot("PROD");
    config->setServer(m_defaultServer);
    config->setMaxJsonSize(1000);

    DerivedTelemetryProcessor telemetryProcessor(config, std::move(m_httpRequester), telemetryProviders);
    telemetryProcessor.Run();
}

TEST_F(TelemetryProcessorTest, telemetryProcessorFallsbackToDirectWhenProxyFails)
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    Common::HttpRequests::Response response;
    response.status = 200;
    response.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    Common::HttpRequests::Response failed;
    failed.status = 500;
    failed.errorCode = Common::HttpRequests::ResponseErrorCode::REQUEST_FAILED;
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, R"({"Mock":{"key":1}})")).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(HasSubstr("current_proxy"))).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));
    EXPECT_CALL(*m_httpRequester, put(_)).WillOnce(Return(failed)).WillOnce(Return(response));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    auto config = std::make_shared<Common::TelemetryConfigImpl::Config>();
    config->setResourceRoot("PROD");
    config->setServer(m_defaultServer);
    config->setMaxJsonSize(1000);

    DerivedTelemetryProcessor telemetryProcessor(config, std::move(m_httpRequester), telemetryProviders);
    telemetryProcessor.Run();
}

TEST_F(TelemetryProcessorTest, telemetryProcessorFallsbackToDirectWhenProxyReturnsNon200status)
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();
    Common::HttpRequests::Response response;
    response.status = 200;
    response.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    Common::HttpRequests::Response failed;
    failed.status = 500;
    failed.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, R"({"Mock":{"key":1}})")).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(HasSubstr("current_proxy"))).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));
    EXPECT_CALL(*m_httpRequester, put(_)).WillOnce(Return(failed)).WillOnce(Return(response));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    auto config = std::make_shared<Common::TelemetryConfigImpl::Config>();
    config->setResourceRoot("PROD");
    config->setServer(m_defaultServer);
    config->setMaxJsonSize(1000);

    DerivedTelemetryProcessor telemetryProcessor(config, std::move(m_httpRequester), telemetryProviders);
    telemetryProcessor.Run();
}

TEST_F(TelemetryProcessorTest, telemetryProcessorDoesNotProcessLargeJsonFromSingleProvider)
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    std::string longString = std::string(m_config->getMaxJsonSize(), '.');

    std::stringstream ss;
    ss << R"({"key":")" << longString << R"("})";

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(ss.str()));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    DerivedTelemetryProcessor telemetryProcessor(m_config, std::move(m_httpRequester), telemetryProviders);

    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock":{"health":1}})", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorDoesNotProcessLargeJsonFromMultipleProviders)
{
    auto mockTelemetryProvider1 = std::make_shared<MockTelemetryProvider>();
    auto mockTelemetryProvider2 = std::make_shared<MockTelemetryProvider>();

    std::string longString = std::string(m_config->getMaxJsonSize() / 2, '.');

    std::stringstream ss;
    ss << R"({"key":")" << longString << R"("})";

    EXPECT_CALL(*mockTelemetryProvider1, getName()).WillOnce(Return("Mock1"));
    EXPECT_CALL(*mockTelemetryProvider1, getTelemetry()).WillOnce(Return(ss.str()));

    EXPECT_CALL(*mockTelemetryProvider2, getName()).WillOnce(Return("Mock2"));
    EXPECT_CALL(*mockTelemetryProvider2, getTelemetry()).WillOnce(Return(ss.str()));

    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _)).Times(testing::AtLeast(1));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider1);
    telemetryProviders.emplace_back(mockTelemetryProvider2);

    DerivedTelemetryProcessor telemetryProcessor(m_config, std::move(m_httpRequester), telemetryProviders);

    EXPECT_THROW(telemetryProcessor.Run(), std::runtime_error);
}
