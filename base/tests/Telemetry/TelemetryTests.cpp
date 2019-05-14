/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include "MockHttpSender.h"
#include "MockTelemetryProvider.h"

#include <Telemetry/TelemetryConfig/Config.h>
#include <Telemetry/TelemetryImpl/Telemetry.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/Telemetry/TelemetryImpl/TelemetryProcessor.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using ::testing::StrictMock;

using namespace Common::HttpSenderImpl;

class TelemetryTest : public ::testing::Test
{
public:
    StrictMock<MockHttpSender> m_httpSender;
    std::vector<std::string> m_additionalHeaders;
    const char* m_data = "{\"mock-telemetry-provider\":{\"mockKey\":\"mockValue\"}}";
    const std::string m_jsonFilePath = "/opt/sophos-spl/base/telemetry/var/telemetry.json";
    std::string m_binaryPath = "/opt/sophos-spl/base/bin/telemetry";
    MockFileSystem* m_mockFileSystem = nullptr;
    Telemetry::TelemetryConfig::Config m_config;

    std::string m_defaultCertPath = Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory(),
        "telemetry_cert.pem");

    std::vector<std::string> m_args = { m_binaryPath, "POST", GL_defaultServer, m_defaultCertPath, "TEST", "extraArg" };

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

        m_additionalHeaders.emplace_back("x-amz-acl:bucket-owner-full-control");

        m_defaultExpectedRequestConfig = std::make_unique<RequestConfig>(RequestType::GET, m_additionalHeaders);
        m_defaultExpectedRequestConfig->setData(m_data);

        {
            m_config.m_verb = RequestType::GET;
            m_config.m_resourceRoute = "PROD";
            m_config.m_certPath = m_defaultCertPath;
            m_config.m_headers = m_additionalHeaders;
            m_config.m_server = GL_defaultServer;
        }
    }

    void TearDown() override { Tests::restoreFileSystem(); }
};

class TelemetryTestRequestTypes : public TelemetryTest, public ::testing::WithParamInterface<RequestType>
{
};

INSTANTIATE_TEST_CASE_P(
    TelemetryTest,
    TelemetryTestRequestTypes,
    ::testing::Values(RequestType::GET, RequestType::POST, RequestType::PUT)); // NOLINT

TEST_P(TelemetryTestRequestTypes, main_httpsRequestReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    auto requestType = GetParam();

    if (requestType == RequestType::GET)
    {
        EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareGetRequestConfig));
    }
    else if (requestType == RequestType::POST)
    {
        EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::ComparePostRequestConfig));
    }
    else if (requestType == RequestType::PUT)
    {
        EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::ComparePutRequestConfig));
    }
    else
    {
        throw std::logic_error("Invalid requestType");
    }

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    m_config.m_verb = requestType;

    Telemetry::TelemetryProcessor telemetryProcessor(m_config, m_httpSender, telemetryProviders);

    telemetryProcessor.Run();
}

TEST_F(TelemetryTest, main_GetRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareRequestConfig));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    m_config.m_verb = RequestType::GET;
    Telemetry::TelemetryProcessor telemetryProcessor(m_config, m_httpSender, telemetryProviders);

    telemetryProcessor.Run();
}

TEST_F(TelemetryTest, main_PostRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareRequestConfig));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    m_config.m_verb = RequestType::POST;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(m_config, m_httpSender, telemetryProviders);

    telemetryProcessor.Run();
}

TEST_F(TelemetryTest, main_certificateDoesNotExist) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(false));

    m_config.m_verb = RequestType::PUT;
    m_config.m_resourceRoute = "DEV";

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();
    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(m_config, m_httpSender, telemetryProviders);

    EXPECT_THROW(telemetryProcessor.Run(), std::runtime_error);
}

TEST_F(TelemetryTest, main_invalidResourceRoot) // NOLINT
{
    m_config.m_verb = RequestType::PUT;
    m_config.m_resourceRoute = "INVALID";

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();
    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(m_config, m_httpSender, telemetryProviders);

    EXPECT_THROW(telemetryProcessor.Run(), std::runtime_error);
}

class TelemetryTestVariableArgs : public TelemetryTest, public ::testing::WithParamInterface<int>
{
};

// TODO: move to different test file
TEST_F(TelemetryTest, telemetryProcessor) // NOLINT
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;

    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryConfig::Config config;

    Telemetry::TelemetryProcessor telemetryProcessor(config, m_httpSender, telemetryProviders);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock":{"key":1}})", json);
}
