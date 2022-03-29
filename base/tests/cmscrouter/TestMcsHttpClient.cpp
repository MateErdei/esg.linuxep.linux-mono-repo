#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockCurlWrapper.h>

#include <cmcsrouter/MCSHttpClient.h>

#include <gtest/gtest.h>

class McsClientTests : public LogInitializedTests
{
};

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

TEST_F(McsClientTests, checkFieldsAreSet)
{
    auto curlWrapper = std::make_shared<StrictMock<MockCurlWrapper>>();
    setCommonExpectations(curlWrapper);
    MCS::MCSHttpClient client("https://mcsUrl","registerToken",curlWrapper);
    client.setID("ThisIsAnMCSID+1001");
    client.setPassword("password");
    Common::HttpRequests::Headers requestHeaders;
    std::variant<std::string, long> urlVariant = "https://mcsUrl/authenticate/endpoint/ThisIsAnMCSID+1001/role/endpoint";

    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_URL, urlVariant)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_SSLVERSION, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_TIMEOUT, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CUSTOMREQUEST, requestTypePost)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_VERBOSE, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAINFO, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasySetOpt(fakeCurlHandle, CURLOPT_CAPATH, _)).WillOnce(Return(CURLE_OK));

    curl_slist* curlHeaders = nullptr;
    curlHeaders = curl_slist_append(curlHeaders, "placeholder only");
    EXPECT_CALL(*curlWrapper, curlSlistAppend(_, "Authorization:Basic VGhpc0lzQW5NQ1NJRCsxMDAxOnBhc3N3b3Jk")).WillOnce(Return(curlHeaders));
    EXPECT_CALL(*curlWrapper, curlSlistAppend(_, "User-Agent:Sophos MCS Client/ Linux sessions registerToken")).WillOnce(Return(curlHeaders));
    EXPECT_CALL(*curlWrapper, curlEasySetOptHeaders(fakeCurlHandle, _)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlSlistFreeAll(_));

    EXPECT_CALL(*curlWrapper, curlEasyPerform(fakeCurlHandle)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*curlWrapper, curlEasyCleanup(fakeCurlHandle));

    EXPECT_CALL(*curlWrapper, curlGetResponseCode(fakeCurlHandle, _))
        .WillOnce(DoAll(SetArgPointee<1>(200), Return(CURLE_OK)));

    EXPECT_CALL(*curlWrapper, curlGlobalCleanup());
    client.sendMessageWithIDAndRole("/authenticate/endpoint/",Common::HttpRequests::RequestType::POST,requestHeaders);
    curl_slist_free_all(curlHeaders);

}