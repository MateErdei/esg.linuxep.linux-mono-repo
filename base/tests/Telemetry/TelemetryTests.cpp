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
#include <modules/Telemetry/TelemetryImpl/TelemetryProcessor.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using Common::HttpSender::RequestConfig;
using Common::HttpSender::RequestConfig;
using Common::HttpSender::RequestType;

class TelemetryTest : public ::testing::Test
{
public:
    std::shared_ptr<StrictMock<MockHttpSender>> m_httpSender;
    std::vector<std::string> m_additionalHeaders;
    const char* m_data = "{\"mock-telemetry-provider\":{\"mockKey\":\"mockValue\"}}";
    std::string m_binaryPath = "/opt/sophos-spl/base/bin/telemetry";
    MockFileSystem* m_mockFileSystem = nullptr;

    std::vector<std::string> m_args = {m_binaryPath, "POST", Common::HttpSender::g_defaultServer, Common::HttpSender::g_defaultCertPath, "TEST", "extraArg"};

    std::shared_ptr<RequestConfig> m_getRequestConfig;
    std::shared_ptr<RequestConfig> m_postRequestConfig;
    std::shared_ptr<RequestConfig> m_putRequestConfig;

    int CompareRequestConfig(const std::shared_ptr<RequestConfig>& requestConfig)
    {
        RequestType requestType = requestConfig->getRequestType();
        std::shared_ptr<RequestConfig> expectedRequestConfig;
        switch(requestType)
        {
            case (RequestType::GET):
                expectedRequestConfig = m_getRequestConfig;
                break;
            case(RequestType::POST):
                expectedRequestConfig = m_postRequestConfig;
                break;
            case(RequestType::PUT):
                expectedRequestConfig = m_putRequestConfig;
                break;
            default:
                throw std::logic_error("Unexpected request type");
        }

        EXPECT_EQ(requestConfig->getRequestType(), expectedRequestConfig->getRequestType());
        EXPECT_EQ(requestConfig->getCertPath(), expectedRequestConfig->getCertPath());
        EXPECT_EQ(requestConfig->getAdditionalHeaders(), expectedRequestConfig->getAdditionalHeaders());
        EXPECT_EQ(requestConfig->getData(), expectedRequestConfig->getData());
        EXPECT_EQ(requestConfig->getServer(), expectedRequestConfig->getServer());

        return 0;
    }

    void SetUp() override
    {
        std::unique_ptr<MockFileSystem> mockfileSystem(new StrictMock<MockFileSystem>());
        m_mockFileSystem = mockfileSystem.get();
        Tests::replaceFileSystem(std::move(mockfileSystem));

        m_httpSender = std::make_shared<StrictMock<MockHttpSender>>();
        m_additionalHeaders.emplace_back("x-amz-acl:bucket-owner-full-control");

        m_getRequestConfig = std::make_shared<RequestConfig>("GET", m_additionalHeaders, Common::HttpSender::g_defaultServer);
        m_getRequestConfig->setData(m_data);
        m_postRequestConfig = std::make_shared<RequestConfig>("POST", m_additionalHeaders, Common::HttpSender::g_defaultServer);
        m_postRequestConfig->setData(m_data);
        m_putRequestConfig = std::make_shared<RequestConfig>("PUT", m_additionalHeaders, Common::HttpSender::g_defaultServer);
        m_putRequestConfig->setData(m_data);
    }

    void TearDown() override
    {
        Tests::restoreFileSystem();
    }
};

class TelemetryTestRequestTypes : public TelemetryTest,
                                  public ::testing::WithParamInterface<std::string> {

};

INSTANTIATE_TEST_CASE_P(TelemetryTest, TelemetryTestRequestTypes, ::testing::Values("GET", "POST", "PUT")); // NOLINT

TEST_P(TelemetryTestRequestTypes, main_entry_httpsRequestReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(Common::HttpSender::g_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareRequestConfig));
    EXPECT_CALL(*m_mockFileSystem, writeFile(_, _)).WillOnce(Return());

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    std::vector<std::string> arguments = {m_binaryPath, GetParam(), Common::HttpSender::g_defaultServer, Common::HttpSender::g_defaultCertPath};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode);
}

TEST_F(TelemetryTest, main_entry_GetRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(Common::HttpSender::g_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareRequestConfig));
    EXPECT_CALL(*m_mockFileSystem, writeFile(_, _)).WillOnce(Return());

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    std::vector<std::string> arguments = {m_binaryPath, "GET"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode);
}

TEST_F(TelemetryTest, main_entry_PostRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(Common::HttpSender::g_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, doHttpsRequest(_)).WillOnce(Invoke(this, &TelemetryTest::CompareRequestConfig));
    EXPECT_CALL(*m_mockFileSystem, writeFile(_, _)).WillOnce(Return());

    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"mockKey":"mockValue"})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("mock-telemetry-provider"));

    std::vector<std::string> arguments = {m_binaryPath, "POST"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode);
}

TEST_F(TelemetryTest, main_entry_InvalidHttpRequestReturnsFailure) // NOLINT
{
    std::vector<std::string> arguments = {m_binaryPath, "DANCE"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 1;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode);
}

TEST_F(TelemetryTest, main_entry_certificateDoesNotExist) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(Common::HttpSender::g_defaultCertPath)).WillOnce(Return(false));

    std::vector<std::string> arguments = {m_binaryPath, "PUT", Common::HttpSender::g_defaultServer, Common::HttpSender::g_defaultCertPath};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 1;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode);
}

class TelemetryTestVariableArgs : public TelemetryTest,
        public ::testing::WithParamInterface<int> {

};

INSTANTIATE_TEST_CASE_P(TelemetryTest, TelemetryTestVariableArgs, ::testing::Values(1,6)); // NOLINT


TEST_P(TelemetryTestVariableArgs, main_entry_HttpRequestReturnsFailure) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, readlink(_)).WillRepeatedly(Return(""));
    std::vector<char*> argv;
    for (int i=0; i < GetParam(); ++i)
    {
        argv.emplace_back(const_cast<char*>(m_args[i].c_str()));
    }

    int expectedErrorCode = 1;

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders);

    EXPECT_EQ(Telemetry::main(argv.size(), argv.data(), m_httpSender, telemetryProcessor), expectedErrorCode);
}

TEST(TelemetryProcessor, telemetryProcessor) // NOLINT
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;

    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock":{"key":1}})", json);
}
