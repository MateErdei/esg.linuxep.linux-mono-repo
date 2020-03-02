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
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <curl/curl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

        m_defaultCertPath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory(),
            "telemetry_cert.pem");
    }
};

TEST_F(HttpSenderTest, getRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, _, _)).Times(2).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, CURLOPT_SSLVERSION,
            {CURL_SSLVERSION_TLSv1_2})).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig getRequestConfig(
        RequestType::GET, m_additionalHeaders, m_defaultServer, m_defaultPort, m_defaultCertPath, "linux/dev");

    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, postRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, _, _)).Times(3).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, CURLOPT_SSLVERSION,
            {CURL_SSLVERSION_TLSv1_2})).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig postRequestConfig(
        RequestType::POST, m_additionalHeaders, m_defaultServer, m_defaultPort, m_defaultCertPath, "linux/prod");

    EXPECT_EQ(m_httpSender->doHttpsRequest(postRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, putRequest) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, _, _)).Times(4).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, CURLOPT_SSLVERSION,
            {CURL_SSLVERSION_TLSv1_2})).WillOnce(Return(m_succeededResult));
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

TEST_F(HttpSenderTest, putRequestWithImplicitCertPath) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, _, _)).Times(4).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, CURLOPT_SSLVERSION,
            {CURL_SSLVERSION_TLSv1_2})).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());

    RequestConfig putRequestConfig(
        RequestType::PUT, m_additionalHeaders, m_defaultServer, m_defaultPort, "", m_defaultResourceRoot);

    EXPECT_EQ(m_httpSender->doHttpsRequest(putRequestConfig), m_succeededResult);
}

TEST_F(HttpSenderTest, getRequest_AdditionalHeaderSuccess) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_, _)).WillOnce(Return(&m_headers));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOptHeaders(_, _)).WillOnce(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, _, _)).Times(3).WillRepeatedly(Return(m_succeededResult));
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
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, _, _)).Times(3).WillRepeatedly(Return(m_succeededResult));
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
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_, _)).WillOnce(Return(nullptr));
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

TEST_F(HttpSenderTest, getRequest_FailsToSetCurlOptionsStillDoesGlobalCleanup) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, _, _)).WillRepeatedly(Return(m_succeededResult));
    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, CURLOPT_SSLVERSION,
            {CURL_SSLVERSION_TLSv1_2})).WillOnce(Return(m_failedResult));
    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(_)).WillOnce(Return(m_strerror.c_str()));
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
//
//TEST_F(HttpSenderTest, getRequest_FailsToSetCurlOptionsStillFreesAllHeaders) // NOLINT
//{
//    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
//    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_, _)).WillOnce(Return(&m_headers));
//    EXPECT_CALL(*m_curlWrapper, curlEasySetOptHeaders(_, _)).WillOnce(Return(m_succeededResult));
//    EXPECT_CALL(*m_curlWrapper, curlEasySetOpt(_, _, "uri").WillRepeatedly(Return(m_succeededResult));
//    EXPECT_CALL(*m_curlWrapper, curlEasyPerform(_)).WillOnce(Return(m_failedResult));
//    EXPECT_CALL(*m_curlWrapper, curlEasyStrError(m_failedResult)).WillOnce(Return(m_strerror.c_str()));
//    EXPECT_CALL(*m_curlWrapper, curlSlistFreeAll(_));
//    EXPECT_CALL(*m_curlWrapper, curlEasyCleanup(_));
//    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());
//
//    m_additionalHeaders.emplace_back("testHeader");
//
//    RequestConfig getRequestConfig(
//        RequestType::GET,
//        m_additionalHeaders,
//        m_defaultServer,
//        m_defaultPort,
//        m_defaultCertPath,
//        m_defaultResourceRoot);
//
//    EXPECT_EQ(m_httpSender->doHttpsRequest(getRequestConfig), m_failedResult);
//}

TEST_F(HttpSenderTest, getRequest_curlSlistAppendReturnsNullThrowsException) // NOLINT
{
    EXPECT_CALL(*m_curlWrapper, curlEasyInit()).WillOnce(Return(&m_curlHandle));
    EXPECT_CALL(*m_curlWrapper, curlSlistAppend(_, _)).WillOnce(Return(nullptr));
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

/*
 * This test can be used to manually debug the CurlWrapper using a locally hosted http server
 * (see https://wiki.sophos.net/display/VT/Run+the+Telemetry+Executable for steps on how to set that up)
 */

/*
class FakeCurlWrapper2 : public Common::HttpSenderImpl::CurlWrapper
{
public:
    FakeCurlWrapper2():m_answer{CURLE_OK}{}
    CURLcode curlEasyPerform(CURL*handle) override
    {
        return Common::HttpSenderImpl::CurlWrapper::curlEasyPerform(handle);
        return m_answer;
    }
    curl_slist* curlSlistAppend(curl_slist* list, const std::string& value) override
    {
        if (value =="breakhere")
        {
            return nullptr;
        }
        return Common::HttpSenderImpl::CurlWrapper::curlSlistAppend(list, value);
    }

    CURLcode curlEasySetOpt(CURL* handle, CURLoption option, const std::string& parameter) override
    {
        if ( parameter == "invalidcert")
        {
            return CURLE_BAD_CONTENT_ENCODING;
        }
        return Common::HttpSenderImpl::CurlWrapper::curlEasySetOpt(handle, option, parameter);
    }

    void setAnswer( CURLcode code)
    {
        m_answer = code;
    }
    CURLcode m_answer;
};

TEST_F(HttpSenderTest, getRequest_WithMultipleHeaders) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    FakeCurlWrapper2 * fk = new FakeCurlWrapper2();
    std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWr{fk};
    EXPECT_CALL(*m_curlWrapper, curlGlobalCleanup());
    m_httpSender = std::make_shared<HttpSender>(curlWr);

    m_additionalHeaders.emplace_back("header1");
    m_additionalHeaders.emplace_back("header2");
    m_additionalHeaders.emplace_back("header3");
    m_additionalHeaders.emplace_back("header4");
    m_additionalHeaders.emplace_back("header5");

    RequestConfig req("GET", m_additionalHeaders, "localhost", GL_defaultPort, "/tmp/cert.pem", ResourceRoot::TEST);
    m_additionalHeaders[0][3]='k';
    EXPECT_EQ(m_httpSender->doHttpsRequest(req), 77);

    EXPECT_EQ(m_httpSender->doHttpsRequest(req), 77);


    RequestConfig req1("GET", m_additionalHeaders, "localhost", GL_defaultPort, "/tmp/cert.pem", ResourceRoot::TEST);
    EXPECT_EQ(m_httpSender->doHttpsRequest(req1), 61);

    m_additionalHeaders.emplace_back("breakhere");
    m_additionalHeaders.emplace_back("donotrunthis");
    RequestConfig req2("GET", m_additionalHeaders, "localhost", GL_defaultPort, "/tmp/cert.pem", ResourceRoot::TEST);
    EXPECT_EQ(m_httpSender->doHttpsRequest(req2), CURLE_FAILED_INIT);

    m_additionalHeaders.clear();
    fk->setAnswer(CURLE_BAD_CONTENT_ENCODING);
    RequestConfig req3("GET", m_additionalHeaders, "localhost", GL_defaultPort, "/tmp/cert.pem", ResourceRoot::TEST);
    EXPECT_EQ(m_httpSender->doHttpsRequest(req3), 77);
}
*/