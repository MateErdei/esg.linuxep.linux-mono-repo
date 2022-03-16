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

TEST_F(HttpRequesterImplTests, clientPerformsGetRequest)
{
    std::string fakeCurlHandle = "a fake curl handle";
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    EXPECT_CALL(*curlWrapper, curlGlobalInit(_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyInit()).WillOnce(Return(&fakeCurlHandle));
    EXPECT_CALL(*curlWrapper, curlEasyReset(&fakeCurlHandle)).Times(1);
    std::variant<std::string, long> url = "https://sophos.com";
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(&fakeCurlHandle, CURLOPT_URL, url)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(&fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(&fakeCurlHandle, CURLOPT_CUSTOMREQUEST, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(&fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(&fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(&fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(&fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(&fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(&fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));
    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());

    HttpRequesterImpl client(curlWrapper);
    Response response = client.get(RequestConfig{ .url = "https://sophos.com" });
    ASSERT_EQ(response.status, 200);
}