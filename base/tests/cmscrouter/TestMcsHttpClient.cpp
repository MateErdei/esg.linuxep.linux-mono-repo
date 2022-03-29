#include <tests/Common/Helpers/LogInitializedTests.h>

#include <cmcsrouter/MCSHttpClient.h>

#include <gtest/gtest.h>

class McsClientTests : public LogInitializedTests
{
};

//TEST_F(McsClientTests, jwtToken)
//{
//    MCS::MCSHttpClient client("https://mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com/sophos/management/ep","965dadea10d403a21a30bdf2572e057b7a1c27e950abbc4638f40682cc6bbf5d");
//    client.setID("edd458c1-ac08-a4d6-eb18-0064282831e3");
//    client.setPassword("aZJ/RpzxoYQj6/vF");
//    Common::HttpRequests::Headers requestHeaders;
//    client.setCertPath("/mnt/filer6/linux/SSPL/tools/setup_sspl/certs/qa_region_certs/hmr-qa-sha256.pem");
//    requestHeaders.insert({"Content-Type","application/xml; charset=utf-8"});
//    Common::HttpRequests::Response response = client.sendMessageWithIDAndRole("/authenticate/endpoint/",Common::HttpRequests::RequestType::POST,requestHeaders);
//    EXPECT_EQ(response.status,200);
//}

TEST_F(McsClientTests, ){}