/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include "MockHttpSender.h"

#include <Telemetry/TelemetryImpl/Telemetry.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::StrictMock;

class TelemetryTest : public ::testing::Test
{
public:
    std::shared_ptr<StrictMock<MockHttpSender>> m_httpSender;
    std::string m_server = "localhost";
    std::vector<std::string> m_additionalHeaders;
    std::string m_data = "{ telemetryKey : telemetryValue }";
    std::string m_customCertPath = "/tmp/telemetry_cert.pem";
    std::string m_defaultCertPath = "/opt/sophos-spl/base/etc/sophosspl/telemetry_cert.pem";
    std::string m_binaryPath = "/opt/sophos-spl/base/bin/telemetry";
    MockFileSystem* m_mockFileSystem = nullptr;

    std::vector<std::string> m_args = {m_binaryPath, "POST", m_server, m_defaultCertPath, "extraArg"};


    void SetUp() override
    {
        std::unique_ptr<MockFileSystem> mockfileSystem(new StrictMock<MockFileSystem>());
        m_mockFileSystem = mockfileSystem.get();
        Tests::replaceFileSystem(std::move(mockfileSystem));

        m_httpSender = std::make_shared<StrictMock<MockHttpSender>>();
        m_additionalHeaders.emplace_back("x-amz-acl:bucket-owner-full-control");
    }

    void TearDown() override
    {
        Tests::restoreFileSystem();
    }
};

TEST_F(TelemetryTest, main_entry_GetRequestReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, setServer(m_server));
    EXPECT_CALL(*m_httpSender, getRequest(_, m_defaultCertPath));

    std::vector<std::string> arguments = {m_binaryPath, "GET", m_server, m_defaultCertPath};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data(), m_httpSender), expectedErrorCode);
}

TEST_F(TelemetryTest, main_entry_PostRequestReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, setServer(m_server));
    EXPECT_CALL(*m_httpSender, postRequest(_, m_data, m_defaultCertPath));

    std::vector<std::string> arguments = {m_binaryPath, "POST", m_server, m_defaultCertPath};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data(), m_httpSender), expectedErrorCode);
}

TEST_F(TelemetryTest, main_entry_GetRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, getRequest(_, m_defaultCertPath));

    std::vector<std::string> arguments = {m_binaryPath, "GET"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data(), m_httpSender), expectedErrorCode);
}

TEST_F(TelemetryTest, main_entry_PostRequestWithOneArgReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_httpSender, postRequest(_, m_data, m_defaultCertPath));

    std::vector<std::string> arguments = {m_binaryPath, "POST"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data(), m_httpSender), expectedErrorCode);
}

TEST_F(TelemetryTest, main_entry_InvalidHttpRequestReturnsFailure) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultCertPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readlink(_)).WillRepeatedly(Return(""));
    std::vector<std::string> arguments = {m_binaryPath, "DANCE"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 1;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data(), m_httpSender), expectedErrorCode);
}

class TelemetryTestVariableArgs : public TelemetryTest,
        public ::testing::WithParamInterface<int> {

};

INSTANTIATE_TEST_CASE_P(TelemetryTest, TelemetryTestVariableArgs, ::testing::Values(1,5)); // NOLINT


TEST_P(TelemetryTestVariableArgs, main_entry_HttpRequestReturnsFailure) // NOLINT
{
    EXPECT_CALL(*m_mockFileSystem, readlink(_)).WillRepeatedly(Return(""));
    std::vector<char*> argv;
    for (int i=0; i < GetParam(); ++i)
    {
        argv.emplace_back(const_cast<char*>(m_args[i].c_str()));
    }

    int expectedErrorCode = 1;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data(), m_httpSender), expectedErrorCode);
}