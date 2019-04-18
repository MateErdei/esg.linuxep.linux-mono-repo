/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include "MockHttpSender.h"

#include <Telemetry/TelemetryImpl/Telemetry.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::StrictMock;

class TelemetryTest : public ::testing::Test
{
public:
    std::shared_ptr<StrictMock<MockHttpSender>> m_httpSender;
    std::string m_server = "localhost";
    std::string m_port = "4443";
    std::vector<std::string> m_additionalHeaders;
    std::string m_jsonStruct = "{ telemetryKey : telemetryValue }";

    std::vector<std::string> m_args = {"/opt/sophos-spl/base/bin/telemetry", "POST", "localhost", "1234", "extraArg"};

    void SetUp() override
    {
        m_httpSender = std::make_shared<StrictMock<MockHttpSender>>();
        m_additionalHeaders.emplace_back("x-amz-acl:bucket-owner-full-control");
    }
};

TEST_F(TelemetryTest, main_entry_GetRequestReturnsSuccess) // NOLINT
{
    EXPECT_CALL(*m_httpSender, setServer(m_server));
    EXPECT_CALL(*m_httpSender, setPort(std::stoi(m_port)));
    EXPECT_CALL(*m_httpSender, get_request(m_additionalHeaders));

    std::vector<std::string> arguments = {"/opt/sophos-spl/base/bin/telemetry", "GET", m_server, m_port};

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
    EXPECT_CALL(*m_httpSender, setServer(m_server));
    EXPECT_CALL(*m_httpSender, setPort(std::stoi(m_port)));
    EXPECT_CALL(*m_httpSender, post_request(m_additionalHeaders, m_jsonStruct));

    std::vector<std::string> arguments = {"/opt/sophos-spl/base/bin/telemetry", "POST", m_server, m_port};

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
    EXPECT_CALL(*m_httpSender, get_request(m_additionalHeaders));

    std::vector<std::string> arguments = {"/opt/sophos-spl/base/bin/telemetry", "GET"};

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
    EXPECT_CALL(*m_httpSender, post_request(m_additionalHeaders, m_jsonStruct));

    std::vector<std::string> arguments = {"/opt/sophos-spl/base/bin/telemetry", "POST"};

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
    std::vector<std::string> arguments = {"/opt/sophos-spl/base/bin/telemetry", "DANCE"};

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

INSTANTIATE_TEST_CASE_P(TelemetryTest,
                        TelemetryTestVariableArgs, ::testing::Values
                        (1,3,5)
);


TEST_P(TelemetryTestVariableArgs, main_entry_HttpRequestReturnsFailure) // NOLINT
{
    std::vector<char*> argv;
    for (int i=0; i < GetParam(); ++i)
    {
        argv.emplace_back(const_cast<char*>(m_args[i].c_str()));
    }

    int expectedErrorCode = 1;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data(), m_httpSender), expectedErrorCode);
}