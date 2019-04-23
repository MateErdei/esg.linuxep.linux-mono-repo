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

#include <curl/curl.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::StrictMock;

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

TEST_F(HttpSenderTest, getRequest) // NOLINT
{
    std::string curlHandle = "validCurlHandle";
    struct curl_slist headers;
    CURLcode result = CURLE_OK;

    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3);
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(3).WillRepeatedly(Return(&headers));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(result));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    std::vector<std::string> additionalHeaders;
    EXPECT_EQ(m_httpSender->getRequest(additionalHeaders), result);
}

TEST_F(HttpSenderTest, postRequest) // NOLINT
{
    std::string curlHandle = "validCurlHandle";
    std::string jsonStruct = "validJsonStruct";
    struct curl_slist headers;
    CURLcode result = CURLE_OK;

    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(4);
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(3).WillRepeatedly(Return(&headers));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(result));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    std::vector<std::string> additionalHeaders;
    EXPECT_EQ(m_httpSender->postRequest(additionalHeaders, jsonStruct), result);
}

TEST_F(HttpSenderTest, getRequest_AdditionalHeaderSuccess) // NOLINT
{
    std::string curlHandle = "validCurlHandle";
    struct curl_slist headers;
    CURLcode result = CURLE_OK;

    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3);
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(4).WillRepeatedly(Return(&headers));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(result));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    std::vector<std::string> additionalHeaders;
    additionalHeaders.emplace_back("testHeader");
    EXPECT_EQ(m_httpSender->getRequest(additionalHeaders), result);
}

TEST_F(HttpSenderTest, getRequest_EasyInitFailureStillDoesGlobalCleanup) // NOLINT
{
    CURLcode result = CURLE_FAILED_INIT;

    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    std::vector<std::string> additionalHeaders;
    EXPECT_EQ(m_httpSender->getRequest(additionalHeaders), result);
}

TEST_F(HttpSenderTest, getRequest_FailureReturnsCorrectCurlCode) // NOLINT
{
    std::string curlHandle = "validCurlHandle";
    struct curl_slist headers;
    CURLcode result = CURLE_FAILED_INIT;
    std::string strerror = "strError";

    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3);
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(AtLeast(3)).WillRepeatedly(Return(&headers));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(result));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrerror(result)).WillOnce(Return(strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    std::vector<std::string> additionalHeaders;
    EXPECT_EQ(m_httpSender->getRequest(additionalHeaders), result);
}