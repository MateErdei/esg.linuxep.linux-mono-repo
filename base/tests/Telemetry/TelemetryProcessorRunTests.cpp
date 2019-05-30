/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include "MockHttpSender.h"
#include "MockTelemetryProvider.h"

#include <Telemetry/TelemetryExeConfigImpl/Config.h>
#include <Telemetry/TelemetryImpl/Telemetry.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/Telemetry/TelemetryImpl/TelemetryProcessor.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using ::testing::StrictMock;

using namespace Common::HttpSenderImpl;

class TelemetryProcessorRunTests : public ::testing::Test
{
public:
    const char* m_defaultServer = "t1.sophosupd.com";
    const int m_defaultPort = 443;
    const std::string m_defaultResourceRoot = "linux/prod";
    const char* m_data = "{\"mock-telemetry-provider\":{\"mockKey\":\"mockValue\"}}";
    const std::string m_jsonFilePath = "/opt/sophos-spl/base/telemetry/var/telemetry.json";
    const std::string m_binaryPath = "/opt/sophos-spl/base/bin/telemetry";

    std::unique_ptr<MockHttpSender> m_httpSender = std::make_unique<StrictMock<MockHttpSender>>();
    std::vector<std::string> m_additionalHeaders;
    MockFileSystem* m_mockFileSystem = nullptr;
    MockFilePermissions* m_mockFilePermissions = nullptr;
    std::shared_ptr<Telemetry::TelemetryConfigImpl::Config> m_config;

    std::string m_defaultCertPath = Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory(),
        "telemetry_cert.pem");

    std::unique_ptr<RequestConfig> m_defaultExpectedRequestConfig;

    int CompareRequestConfig(RequestConfig& requestConfig)
    {
        EXPECT_EQ(requestConfig.getCertPath(), m_defaultExpectedRequestConfig->getCertPath());
        EXPECT_EQ(requestConfig.getAdditionalHeaders(), m_defaultExpectedRequestConfig->getAdditionalHeaders());
        EXPECT_EQ(requestConfig.getData(), m_defaultExpectedRequestConfig->getData());
        EXPECT_EQ(requestConfig.getServer(), m_defaultExpectedRequestConfig->getServer());
        return 0;
    }

    int CompareGetRequestConfig(RequestConfig& requestConfig)
    {
        EXPECT_EQ(requestConfig.getRequestTypeAsString(), "GET");
        return CompareRequestConfig(requestConfig);
    }

    int ComparePostRequestConfig(RequestConfig& requestConfig)
    {
        EXPECT_EQ(requestConfig.getRequestTypeAsString(), "POST");
        return CompareRequestConfig(requestConfig);
    }

    int ComparePutRequestConfig(RequestConfig& requestConfig)
    {
        EXPECT_EQ(requestConfig.getRequestTypeAsString(), "PUT");
        return CompareRequestConfig(requestConfig);
    }

    void SetUp() override
    {
        std::unique_ptr<MockFileSystem> mockfileSystem(new StrictMock<MockFileSystem>());
        m_mockFileSystem = mockfileSystem.get();
        Tests::replaceFileSystem(std::move(mockfileSystem));

        std::unique_ptr<MockFilePermissions> mockfilePermissions(new StrictMock<MockFilePermissions>());
        m_mockFilePermissions = mockfilePermissions.get();
        Tests::replaceFilePermissions(std::move(mockfilePermissions));

        m_additionalHeaders.emplace_back("x-amz-acl:bucket-owner-full-control");

        m_defaultExpectedRequestConfig = std::make_unique<RequestConfig>(
            RequestType::GET,
            m_additionalHeaders,
            m_defaultServer,
            m_defaultPort,
            m_defaultCertPath,
            m_defaultResourceRoot);

        m_defaultExpectedRequestConfig->setData(m_data);

        m_config = std::make_shared<Telemetry::TelemetryConfigImpl::Config>();
        m_config->setVerb("PUT");
        m_config->setResourceRoot("PROD");
        m_config->setTelemetryServerCertificatePath(m_defaultCertPath);
        m_config->setHeaders(m_additionalHeaders);
        m_config->setServer(m_defaultServer);
        m_config->setMaxJsonSize(1000);
    }

    void TearDown() override
    {
        Tests::restoreFilePermissions();
        Tests::restoreFileSystem();
    }
};

class TelemetryProcessorRunTestRequestTypes : public TelemetryProcessorRunTests,
                                              public ::testing::WithParamInterface<RequestType>
{
};

INSTANTIATE_TEST_CASE_P(
    TelemetryTest,
    TelemetryProcessorRunTestRequestTypes,
    ::testing::Values(RequestType::GET, RequestType::POST, RequestType::PUT)); // NOLINT

TEST_P(TelemetryProcessorRunTestRequestTypes, httpsRequestReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    auto requestType = GetParam();

    if (requestType == RequestType::GET)
    {
        EXPECT_CALL(*m_httpSender, doHttpsRequest(_))
            .WillOnce(Invoke(this, &TelemetryProcessorRunTests::CompareGetRequestConfig));
    }
    else if (requestType == RequestType::POST)
    {
        EXPECT_CALL(*m_httpSender, doHttpsRequest(_))
            .WillOnce(Invoke(this, &TelemetryProcessorRunTests::ComparePostRequestConfig));
    }
    else if (requestType == RequestType::PUT)
    {
        EXPECT_CALL(*m_httpSender, doHttpsRequest(_))
            .WillOnce(Invoke(this, &TelemetryProcessorRunTests::ComparePutRequestConfig));
    }
    else
    {
        throw std::logic_error("Invalid requestType");
    }

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    m_config->setVerb(Common::HttpSenderImpl::RequestConfig::requestTypeToString(requestType));
    Telemetry::TelemetryProcessor telemetryProcessor(m_config, std::move(m_httpSender), telemetryProviders);

    telemetryProcessor.Run();
}

TEST_F(TelemetryProcessorRunTests, GetRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, doHttpsRequest(_))
        .WillOnce(Invoke(this, &TelemetryProcessorRunTests::CompareRequestConfig));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    m_config->setVerb("GET");
    Telemetry::TelemetryProcessor telemetryProcessor(m_config, std::move(m_httpSender), telemetryProviders);

    telemetryProcessor.Run();
}

TEST_F(TelemetryProcessorRunTests, PutRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, doHttpsRequest(_))
        .WillOnce(Invoke(this, &TelemetryProcessorRunTests::CompareRequestConfig));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    m_config->setVerb("PUT");

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(m_config, std::move(m_httpSender), telemetryProviders);

    telemetryProcessor.Run();
}

TEST_F(TelemetryProcessorRunTests, PostRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, doHttpsRequest(_))
        .WillOnce(Invoke(this, &TelemetryProcessorRunTests::CompareRequestConfig));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    m_config->setVerb("POST");

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(m_config, std::move(m_httpSender), telemetryProviders);

    telemetryProcessor.Run();
}

TEST_F(TelemetryProcessorRunTests, certificateDoesNotExist) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(false));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    m_config->setVerb("PUT");
    m_config->setResourceRoot("DEV");

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(m_config, std::move(m_httpSender), telemetryProviders);

    EXPECT_THROW(telemetryProcessor.Run(), std::runtime_error);
}

TEST_F(TelemetryProcessorRunTests, httpRequestFails) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, doHttpsRequest(_)).WillRepeatedly(Return(42));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    m_config->setVerb("PUT");

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(m_config, std::move(m_httpSender), telemetryProviders);

    EXPECT_THROW(telemetryProcessor.Run(), std::runtime_error);
}
