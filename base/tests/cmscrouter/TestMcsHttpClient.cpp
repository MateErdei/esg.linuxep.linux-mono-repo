#include <tests/Common/Helpers/LogInitializedTests.h>

#include <cmcsrouter/MCSHttpClient.h>

#include <gtest/gtest.h>

class McsClientTests : public LogInitializedTests
{
};

TEST_F(McsClientTests, jwtToken)
{
    MCS::MCSHttpClient client("https://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com/sophos/management/ep","027858598e1b2b7b22625bd075a78e220c172d6b725e49db666ec8ad23f953ab");
    client.setID("");
    client.setPassword("");
    Common::HttpRequests::Headers requestHeaders;
    client.setCertPath("/mnt/filer6/linux/SSPL/tools/setup_sspl/certs/qa_region_certs/hmr-qa-sha256.pem");
    requestHeaders.insert({"Content-Type","application/xml; charset=utf-8"});
    Common::HttpRequests::Response response = client.sendMessageWithIDAndRole("/authenticate/endpoint/",Common::HttpRequests::RequestType::POST,requestHeaders);
    EXPECT_EQ(response.status,200);
}