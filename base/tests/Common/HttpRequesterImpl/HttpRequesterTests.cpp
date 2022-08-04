#include "../Helpers/LogInitializedTests.h"
#include "../Helpers/MockCurlWrapper.h"

#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <curl/curl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class HttpRequesterImplTests : public LogInitializedTests
{
};
using namespace Common::HttpRequests;
using namespace Common::HttpRequestsImpl;


namespace
{
    const std::string URL = "https://sophos.com";
    void* fakeCurlHandle = reinterpret_cast<void*>(0xdeadbeef);
    std::variant<std::string, long> requestTypeGet = "GET";
    std::variant<std::string, long> requestTypePut = "PUT";
    std::variant<std::string, long> requestTypePost = "POST";
    std::variant<std::string, long> requestTypeDelete = "DELETE";
    std::variant<std::string, long> requestTypeOptions = "OPTIONS";

    void setCommonExpectations(std::shared_ptr<StrictMock<MockCurlWrapper>> curlWrapper)
    {
        EXPECT_CALL(*curlWrapper, curlGlobalInit(_)).WillOnce(Return(CURLE_OK));
        EXPECT_CALL(*curlWrapper, curlEasyInit()).WillOnce(Return(fakeCurlHandle));
        EXPECT_CALL(*curlWrapper, curlEasyReset(fakeCurlHandle)).Times(1);

        EXPECT_CALL(*curlWrapper, curlEasySetFuncOpt(fakeCurlHandle, CURLOPT_HEADERFUNCTION, _)).WillOnce(Return(CURLE_OK));
        EXPECT_CALL(*curlWrapper, curlEasySetDataOpt(fakeCurlHandle, CURLOPT_HEADERDATA, _)).WillOnce(Return(CURLE_OK));

        EXPECT_CALL(*curlWrapper, curlEasySetFuncOpt(fakeCurlHandle, CURLOPT_DEBUGFUNCTION, _)).WillOnce(Return(CURLE_OK));

        EXPECT_CALL(*curlWrapper, curlEasySetFuncOpt(fakeCurlHandle, CURLOPT_WRITEFUNCTION, _)).WillOnce(Return(CURLE_OK));
        EXPECT_CALL(*curlWrapper, curlEasySetDataOpt(fakeCurlHandle, CURLOPT_WRITEDATA, _)).WillOnce(Return(CURLE_OK));
    }
}

// GET tests

TEST_F(HttpRequesterImplTests, clientPerformsGetRequest)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);

    std::variant<std::string, long> urlVariant = URL;
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypeGet)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.get(RequestConfig{ .url = URL });
    ASSERT_EQ(response.status, 200);
}

TEST_F(HttpRequesterImplTests, clientPerformsGetRequestWithPort)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);

    std::variant<std::string, long> urlVariant = URL;
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypeGet)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_PORT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));


    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.get(RequestConfig{
        .url = URL,
        .port = 9000
    });
    ASSERT_EQ(response.status, 200);
}


TEST_F(HttpRequesterImplTests, clientPerformsGetRequestWithBandwidthLimit)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);
    std::variant<std::string, long> urlVariant = URL;
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypeGet)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_MAX_RECV_SPEED_LARGE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.get(RequestConfig{
        .url = URL,
        .bandwidthLimit = 123
    });
    ASSERT_EQ(response.status, 200);
}

TEST_F(HttpRequesterImplTests, clientPerformsGetRequestWithRedirectsEnabled)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);
    std::variant<std::string, long> urlVariant = URL;
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypeGet)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_FOLLOWLOCATION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_MAXREDIRS, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.get(RequestConfig{
        .url = URL,
        .allowRedirects = true
    });
    ASSERT_EQ(response.status, 200);
}

TEST_F(HttpRequesterImplTests, clientPerformsGetRequestSendingHeaders)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);
    std::variant<std::string, long> urlVariant = URL;
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypeGet)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    // not directly used but need to return something from the mock that can be freed
    curl_slist* curlHeaders = nullptr;
    curlHeaders = curl_slist_append(curlHeaders, "placeholder only");

    // "header1: value1" is what HttpRequests::Headers {{"header1", "value1"}}, in the request, will be converted to
    EXPECT_CALL(*curlWrapper, curlSlistAppend(_, "header1: value1")).WillOnce(Return(curlHeaders));
    EXPECT_CALL(*curlWrapper, curlEasySetOptHeaders(fakeCurlHandle, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));
    EXPECT_CALL(*curlWrapper, curlSlistFreeAll(_));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.get(RequestConfig{
        .url = URL,
        .headers = Common::HttpRequests::Headers {{"header1", "value1"}}
    });
    ASSERT_EQ(response.status, 200);

    // Doesn't really matter about this test only free not being run if there's a failure, it's just here to make sure
    // the sanitizers are happy and don't report a leak which is due to the test only variable curlHeaders being used.
    curl_slist_free_all(curlHeaders);
}


TEST_F(HttpRequesterImplTests, clientPerformsGetRequestToDownloadFileToNamedFile)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);

    std::variant<std::string, long> urlVariant = URL;
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypeGet)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.get(RequestConfig{
        .url = URL,
       .fileDownloadLocation = "/save/to/here/please.txt"
    });
    ASSERT_EQ(response.status, 200);
}


// POST tests

TEST_F(HttpRequesterImplTests, clientPerformsPostRequest)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);

    std::variant<std::string, long> urlVariant = URL;

    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypePost)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.post(RequestConfig{ .url = URL });
    ASSERT_EQ(response.status, 200);
}


// PUT tests

TEST_F(HttpRequesterImplTests, clientPerformsPutRequest)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);

    std::variant<std::string, long> urlVariant = URL;

    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypePut)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.put(RequestConfig{ .url = URL });
    ASSERT_EQ(response.status, 200);
}


// DELETE tests

TEST_F(HttpRequesterImplTests, clientPerformsDeleteRequest)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);

    std::variant<std::string, long> urlVariant = URL;

    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypeDelete)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.del(RequestConfig{ .url = URL });
    ASSERT_EQ(response.status, 200);
}


// OPTIONS tests

TEST_F(HttpRequesterImplTests, clientPerformsOptionsRequest)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);

    std::variant<std::string, long> urlVariant = URL;

    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypeOptions)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_NOPROXY, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.options(RequestConfig{ .url = URL });
    ASSERT_EQ(response.status, 200);
}
