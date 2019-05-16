/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include "MockCurlWrapper.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/HttpSenderImpl/HttpSender.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <curl/curl.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::StrictMock;

using namespace Common::HttpSenderImpl;

class HttpSenderTest : public ::testing::Test
{
public:
    const char* m_defaultServer = "t1.sophosupd.com";
    const int m_defaultPort = 443;
    const std::string m_defaultResourceRoot = "linux/prod";

    std::shared_ptr<StrictMock<MockCurlWrapper>> m_curlWrapper;
    std::shared_ptr<HttpSender> m_httpSender;

    std::vector<std::string> m_additionalHeaders;

    std::string m_curlHandle = "validCurlHandle";
    curl_slist m_headers = curl_slist();
    CURLcode m_succeededResult = CURLE_OK;
    CURLcode m_failedResult = CURLE_FAILED_INIT;
    std::string m_strerror = "Test Error String";
    std::string m_defaultCertPath;

    void SetUp() override
    {
        m_curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();

        EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_succeededResult));

        m_httpSender = std::make_shared<HttpSender>(m_curlWrapper);

        m_defaultCertPath = Common::FileSystem::join(Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory(), "telemetry_cert.pem");
    }
};

TEST_F(HttpSenderTest, getRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_,_,_)).Times(2).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig getRequestConfig(RequestType::GET, m_additionalHeaders, m_defaultServer, m_defaultPort, m_defaultCertPath, "linux/dev");

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, postRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_,_,_)).Times(3).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig postRequestConfig(RequestType::POST, m_additionalHeaders, m_defaultServer, m_defaultPort, m_defaultCertPath, "linux/prod");

    EXPECT_EQ(m_httpSender->doHttpsRequest(postRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, putRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_,_,_)).Times(4).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig putRequestConfig(
        RequestType::PUT,
        m_additionalHeaders,
        m_defaultServer,
        m_defaultPort,
        "/nonDefaultCertPath",
        m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(putRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, getRequest_AdditionalHeaderSuccess) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).WillOnce(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_,_,_)).Times(3).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    m_additionalHeaders.emplace_back("testHeader");

    RequestConfig getRequestConfig(
        RequestType::GET,
        m_additionalHeaders,
        m_defaultServer,
        m_defaultPort,
        m_defaultCertPath,
        m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, getRequest_EasyInitFailureStillDoesGlobalCleanup) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(nullptr));
    //EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig getRequestConfig(
        RequestType::GET,
        m_additionalHeaders,
        m_defaultServer,
        m_defaultPort,
        m_defaultCertPath,
        m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailureReturnsCorrectCurlCode) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_,_,_)).Times(2).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig getRequestConfig(
        RequestType::GET,
        m_additionalHeaders,
        m_defaultServer,
        m_defaultPort,
        m_defaultCertPath,
        m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailsToAppendHeader) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(_)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    m_additionalHeaders.emplace_back("testHeader");

    RequestConfig getRequestConfig(
        RequestType::GET,
        m_additionalHeaders,
        m_defaultServer,
        m_defaultPort,
        m_defaultCertPath,
        m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailsToSetCurlOptionsStillDoesGlobalCleanup) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_,_,_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(_)).Times(2).WillRepeatedly(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig getRequestConfig(
        RequestType::GET,
        m_additionalHeaders,
        m_defaultServer,
        m_defaultPort,
        m_defaultCertPath,
        m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_FailsToSetCurlOptionsStillFreesAllHeaders) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).WillOnce(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_,_,_)).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    m_additionalHeaders.emplace_back("testHeader");

    RequestConfig getRequestConfig(
        RequestType::GET,
        m_additionalHeaders,
        m_defaultServer,
        m_defaultPort,
        m_defaultCertPath,
        m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, getRequest_curlSlistAppendReturnsNullThrowsException) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_,_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(_)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    m_additionalHeaders.emplace_back("testHeader");

    RequestConfig getRequestConfig(
        RequestType::GET,
        m_additionalHeaders,
        m_defaultServer,
        m_defaultPort,
        m_defaultCertPath,
        m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, HttpSender_CurlGlobalInitialisationFailureThrowsExceptionInConstructor) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlGlobalInit(_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    EXPECT_THROW(std::make_shared<HttpSender>(m_curlWrapper), std::runtime_error); // NOLINT
}

TEST_F(HttpSenderTest, putRequest_SetOptFailureThrowsRuntimeError) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_,_,_)).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(_)).Times(2).WillRepeatedly(Return(m_strerror.c_str()));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig putRequestConfig("PUT", m_additionalHeaders, GL_defaultServer, GL_defaultPort, "/nonDefaultCertPath");

    EXPECT_EQ(m_httpSender->doHttpsRequest(putRequestConfig), m_failedResult);
}

TEST_F(HttpSenderTest, putRequest_EasyInitReturnsNullptrThrowsRuntimeError) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(nullptr));
    //EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig putRequestConfig("PUT", m_additionalHeaders, GL_defaultServer, GL_defaultPort, "/nonDefaultCertPath");

    EXPECT_EQ(m_httpSender->doHttpsRequest(putRequestConfig), m_failedResult);
}