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
    int m_port = HTTP_PORT;
    std::vector<std::string> m_additionalHeaders;

    std::shared_ptr<HttpSender> m_httpSender;

    std::string m_curlHandle = "validCurlHandle";
    std::string m_jsonStruct = "validJsonStruct";
    struct curl_slist m_headers = curl_slist();
    CURLcode m_succeededResult = CURLE_OK;
    CURLcode m_failedResult = CURLE_FAILED_INIT;
    std::string m_strerror = "strError";
    std::string m_certPath = "/opt/sophos-spl/base/etc/telemetry_cert.pem";

    void SetUp() override
    {
        m_curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
        m_httpSender = std::make_shared<HttpSender>(m_server, m_port, m_curlWrapper);
    }
};

TEST_F(HttpSenderTest, getRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3);
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(3).WillRepeatedly(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->getRequest(m_additionalHeaders, m_certPath), m_succeededResult);
}

TEST_F(HttpSenderTest, postRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(4);
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(3).WillRepeatedly(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->postRequest(m_additionalHeaders, m_jsonStruct, m_certPath), m_succeededResult);
}

TEST_F(HttpSenderTest, getRequest_AdditionalHeaderSuccess) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3);
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(4).WillRepeatedly(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    m_additionalHeaders.emplace_back("testHeader");
    EXPECT_EQ(m_httpSender->getRequest(m_additionalHeaders, m_certPath), m_succeededResult);
}

TEST_F(HttpSenderTest, getRequest_EasyInitFailureStillDoesGlobalCleanup) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->getRequest(m_additionalHeaders, m_certPath), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailureReturnsCorrectCurlCode) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3);
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).Times(AtLeast(3)).WillRepeatedly(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrerror(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->getRequest(m_additionalHeaders, m_certPath), m_failedResult);
}
