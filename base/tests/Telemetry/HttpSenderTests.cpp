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
    std::shared_ptr<HttpSender> m_httpSender;

    std::vector<std::string> m_additionalHeaders;

    std::string m_curlHandle = "validCurlHandle";
    struct curl_slist m_headers = curl_slist();
    CURLcode m_succeededResult = CURLE_OK;
    CURLcode m_failedResult = CURLE_FAILED_INIT;
    std::string m_strerror = "Test Error String";

    std::shared_ptr<RequestConfig> m_getRequestConfig;
    std::shared_ptr<RequestConfig> m_postRequestConfig;
    std::shared_ptr<RequestConfig> m_putRequestConfig;

    void SetUp() override
    {
        m_curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
        m_httpSender = std::make_shared<HttpSender>(m_curlWrapper);

        m_getRequestConfig =  std::make_shared<RequestConfig>("GET", m_additionalHeaders, g_defaultServer, g_defaultPort, g_defaultCertPath, ResourceRoot::DEV);
        m_postRequestConfig =  std::make_shared<RequestConfig>("POST", m_additionalHeaders, g_defaultServer, g_defaultPort, g_defaultCertPath, ResourceRoot::PROD);
        m_putRequestConfig =  std::make_shared<RequestConfig>("PUT", m_additionalHeaders, g_defaultServer, g_defaultPort, "/nonDefaultCertPath");
    }
};

TEST_F(HttpSenderTest, getRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(2).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->httpsRequest(m_getRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, postRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->httpsRequest(m_postRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, putRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(4).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->httpsRequest(m_putRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, getRequest_AdditionalHeaderSuccess) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).WillOnce(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(3).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    m_additionalHeaders.emplace_back("testHeader");

    std::shared_ptr<RequestConfig> getRequestConfig = std::make_shared<RequestConfig>(
        "GET", m_additionalHeaders
    );

    EXPECT_EQ(m_httpSender->httpsRequest(getRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, getRequest_EasyInitFailureStillDoesGlobalCleanup) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->httpsRequest(m_getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_GlobalInitFailureStillDoesGlobalCleanup) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrerror(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->httpsRequest(m_getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailureReturnsCorrectCurlCode) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).Times(2).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrerror(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->httpsRequest(m_getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailsToAppendHeader) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    m_additionalHeaders.emplace_back("testHeader");

    std::shared_ptr<RequestConfig> getRequestConfig = std::make_shared<RequestConfig>(
        "GET", m_additionalHeaders
    );

    EXPECT_EQ(m_httpSender->httpsRequest(getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailsToSetCurlOptionsStillDoesGlobalCleanup) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrerror(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_EQ(m_httpSender->httpsRequest(m_getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailsToSetCurlOptionsStillFreesAllHeaders) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).WillOnce(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasySetopt(_,_,_)).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrerror(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    m_additionalHeaders.emplace_back("testHeader");

    std::shared_ptr<RequestConfig> getRequestConfig = std::make_shared<RequestConfig>(
        "GET", m_additionalHeaders
    );

    EXPECT_EQ(m_httpSender->httpsRequest(getRequestConfig), m_failedResult);
}

