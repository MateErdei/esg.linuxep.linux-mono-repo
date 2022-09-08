#include <cmcsrouter/MCSHttpClient.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockHttpRequester.h>

class McsClientTests : public LogInitializedTests
{
};

TEST_F(McsClientTests, checkFieldsAreSet)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();

    MCS::MCSHttpClient client("https://mcsUrl","registerToken",httpRequester);
    client.setID("ThisIsAnMCSID+1001");
    client.setPassword("password");
    client.setVersion("1.0.0");
    Common::HttpRequests::Headers requestHeaders;

    Common::HttpRequests::RequestConfig expected;
    Common::HttpRequests::Headers expectedHeaders;
    expectedHeaders.insert({"Authorization","Basic VGhpc0lzQW5NQ1NJRCsxMDAxOnBhc3N3b3Jk"});
    expectedHeaders.insert({"User-Agent","Sophos MCS Client/1.0.0 Linux sessions registerToken"});
    expected.url = "https://mcsUrl/authenticate/endpoint/ThisIsAnMCSID+1001/role/endpoint";
    expected.headers = expectedHeaders;
    expected.timeout = 60;
    Common::HttpRequests::Response expectedResponse;
    EXPECT_CALL(*httpRequester, post(expected)).WillOnce(Return(expectedResponse));

    client.sendMessageWithIDAndRole("/authenticate/endpoint/",Common::HttpRequests::RequestType::POST,requestHeaders);
}


TEST_F(McsClientTests, checkProxyFieldsAreSet)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();

    MCS::MCSHttpClient client("https://mcsUrl","registerToken",httpRequester);
    client.setID("ThisIsAnMCSID+1001");
    client.setPassword("password");
    client.setVersion("1.0.0");
    client.setProxyInfo("10.55.36.66","user","password");
    Common::HttpRequests::Headers requestHeaders;

    Common::HttpRequests::RequestConfig expected;
    Common::HttpRequests::Headers expectedHeaders;
    expectedHeaders.insert({"Authorization","Basic VGhpc0lzQW5NQ1NJRCsxMDAxOnBhc3N3b3Jk"});
    expectedHeaders.insert({"User-Agent","Sophos MCS Client/1.0.0 Linux sessions registerToken"});
    expected.url = "https://mcsUrl/authenticate/endpoint/ThisIsAnMCSID+1001/role/endpoint";
    expected.headers = expectedHeaders;
    expected.proxy = "10.55.36.66";
    expected.proxyUsername = "user";
    expected.proxyPassword = "password";
    expected.timeout = 60;
    Common::HttpRequests::Response expectedResponse;
    EXPECT_CALL(*httpRequester, get(expected)).WillOnce(Return(expectedResponse));

    client.sendMessageWithIDAndRole("/authenticate/endpoint/",Common::HttpRequests::RequestType::GET,requestHeaders);

}

TEST_F(McsClientTests, setCertPath)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();

    MCS::MCSHttpClient client("https://mcsUrl","registerToken",httpRequester);
    client.setID("ThisIsAnMCSID+1001");
    client.setPassword("password");
    client.setVersion("1.0.0");
    client.setCertPath("/tmp/random");
    Common::HttpRequests::Headers requestHeaders;

    Common::HttpRequests::RequestConfig expected;
    Common::HttpRequests::Headers expectedHeaders;
    expectedHeaders.insert({"Authorization","Basic VGhpc0lzQW5NQ1NJRCsxMDAxOnBhc3N3b3Jk"});
    expectedHeaders.insert({"User-Agent","Sophos MCS Client/1.0.0 Linux sessions registerToken"});
    expected.url = "https://mcsUrl/authenticate/endpoint/ThisIsAnMCSID+1001/role/endpoint";
    expected.headers = expectedHeaders;
    expected.certPath = "/tmp/random";
    expected.timeout = 60;
    Common::HttpRequests::Response expectedResponse;
    EXPECT_CALL(*httpRequester, put(expected)).WillOnce(Return(expectedResponse));

    client.sendMessageWithIDAndRole("/authenticate/endpoint/",Common::HttpRequests::RequestType::PUT,requestHeaders);
}