/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include "MockHttpSender.h"
#include "MockTelemetryProvider.h"

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

class TelemetryTest : public ::testing::Test
{
public:
    StrictMock<MockHttpSender> m_httpSender;
    std::vector<std::string> m_additionalHeaders;
    const char* m_data = R"("{"mock-telemetry-provider":{"mockKey":"mockValue"}}")";
    const std::string m_jsonFilePath = "/opt/sophos-spl/base/telemetry/var/telemetry.json";
    std::string m_binaryPath = "/opt/sophos-spl/base/bin/telemetry";
    MockFilePermissions* m_mockFilePermissions = nullptr;
    MockFileSystem* m_mockFileSystem = nullptr;
    const std::size_t m_maxJsonBytes = 1000;

    std::string m_defaultCertPath = Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory(),
        "telemetry_cert.pem");

    std::vector<std::string> m_args = { m_binaryPath, "POST", GL_defaultServer, m_defaultCertPath, "TEST", "extraArg" };

    std::unique_ptr<RequestConfig> m_defaultRequestConfig;

    int CompareRequestConfig(RequestConfig& requestConfig)
    {
        EXPECT_EQ(requestConfig.getCertPath(), m_defaultRequestConfig->getCertPath());
        EXPECT_EQ(requestConfig.getAdditionalHeaders(), m_defaultRequestConfig->getAdditionalHeaders());
        EXPECT_EQ(requestConfig.getData(), m_defaultRequestConfig->getData());
        EXPECT_EQ(requestConfig.getServer(), m_defaultRequestConfig->getServer());
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

        m_defaultRequestConfig = std::make_unique<RequestConfig>("GET", m_additionalHeaders);
        m_defaultRequestConfig->setData(m_data);
    }

    void TearDown() override { Tests::restoreFileSystem(); }
};

class TelemetryTestRequestTypes : public TelemetryTest, public ::testing::WithParamInterface<std::string>
{
};

INSTANTIATE_TEST_CASE_P(TelemetryTest, TelemetryTestRequestTypes, ::testing::Values("GET", "POST", "PUT")); // NOLINT

TEST_P(TelemetryTestRequestTypes, main_httpsRequestReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    std::string requestType = GetParam();

    if (requestType == "GET")
    {
        EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareGetRequestConfig));
    }
    else if (requestType == "POST")
    {
        EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::ComparePostRequestConfig));
    }
    else if (requestType == "PUT")
    {
        EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::ComparePutRequestConfig));
    }
    else
    {
        throw std::logic_error("Invalid requestType");
    }

    std::vector<std::string> arguments = { m_binaryPath, requestType, GL_defaultServer, m_defaultCertPath, "PROD" };

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode);
}

TEST_F(TelemetryTest, main_GetRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareRequestConfig));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    std::vector<std::string> arguments = { m_binaryPath, "GET" };

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode);
}

TEST_F(TelemetryTest, main_PostRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareRequestConfig));

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    std::vector<std::string> arguments = { m_binaryPath, "POST" };

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode); // NOLINT
}

TEST_F(TelemetryTest, main_InvalidArgsReturnsFailure) // NOLINT
{
    std::vector<std::string> arguments = { m_binaryPath, "DANCE" };

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();
    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);

    EXPECT_THROW(
        Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), std::runtime_error); // NOLINT
}

TEST_F(TelemetryTest, main_certificateDoesNotExist) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(false));

    std::vector<std::string> arguments = { m_binaryPath, "PUT", GL_defaultServer, m_defaultCertPath, "DEV" };

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();
    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);

    EXPECT_THROW(
        Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), std::runtime_error); // NOLINT
}

TEST_F(TelemetryTest, main_invalidResourceRoot) // NOLINT
{
    std::vector<std::string> arguments = { m_binaryPath, "PUT", GL_defaultServer, m_defaultCertPath, "INVALID" };

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();
    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);

    EXPECT_THROW(
        Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), std::runtime_error); // NOLINT
}

class TelemetryTestVariableArgs : public TelemetryTest, public ::testing::WithParamInterface<int>
{
};

INSTANTIATE_TEST_CASE_P(TelemetryTest, TelemetryTestVariableArgs, ::testing::Values(1, 6)); // NOLINT

TEST_P(TelemetryTestVariableArgs, main_HttpRequestReturnsFailure) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, readlink(_)).WillRepeatedly(Return(""));

    std::vector<char*> argv;
    for (int i = 0; i < GetParam(); ++i)
    {
        argv.emplace_back(const_cast<char*>(m_args[i].c_str()));
    }

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();
    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);

    EXPECT_THROW(
        Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), std::runtime_error); // NOLINT
}
