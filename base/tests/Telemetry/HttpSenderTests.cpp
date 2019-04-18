/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include "MockCurlWrapper.h"

#include <Telemetry/HttpSenderImpl/HttpSender.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::WillOnce;

class HttpSenderTest : public ::testing::Test
{
public:
    std::shared_ptr<StrictMock<MockCurlWrapper>> m_curlWrapper;
    std::string m_server = "testServer.com";
    int m_port = 1234;
    std::shared_ptr<HttpSender> m_httpSender;

    void SetUp() override
    {
        m_curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
        m_httpSender = std::make_shared<HttpSender>(m_server, m_port, m_curlWrapper);
    }
};

TEST_F(HttpSenderTest, get_request_EasyInitFailureStillDoesGlobalCleanup) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(nullptr));
//    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3); // 4 for POST
//    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(AtLeast(3));
//    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_));
//    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
//    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    std::vector<std::string> additionalHeaders;
    additionalHeaders.emplace_back("testHeader");
    m_httpSender->get_request(additionalHeaders);
}