/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Telemetry/TelemetryConfig/Config.h>
#include <modules/Telemetry/TelemetryConfig/TelemetryConfigSerialiser.h>

using ::testing::StrictMock;
using namespace Telemetry::TelemetryConfig;

class TelemetryConfigTest : public ::testing::Test
{
public:
    Config m_config;


    nlohmann::json m_jsonObject;

    void SetUp() override
    {
        MessageRelay messageRelay;
        Proxy proxy;

        m_config.m_server = "localhost";
        m_config.m_verb  = Common::HttpSenderImpl::RequestType::GET;
        m_config.m_externalProcessTimeout = 3;
        m_config.m_headers = {"header1", "header2"};
        m_config.m_maxJsonSize = 10;
        m_config.m_port = 123;
        m_config.m_resourceRoot = "TEST";

        messageRelay.m_url = "relay";
        messageRelay.m_port = 456;
        messageRelay.m_authentication = MessageRelay::Authentication::basic;
        messageRelay.m_certificatePath = "certpath";
        messageRelay.m_username = "relayuser";
        messageRelay.m_password = "relaypw";

        m_config.m_messageRelays.push_back(messageRelay);

        proxy.m_url = "proxy";
        proxy.m_port = 789;
        proxy.m_authentication = MessageRelay::Authentication::basic;
        proxy.m_username = "proxyuser";
        proxy.m_password = "proxypw";

        m_config.m_proxies.push_back(proxy);

        m_jsonObject["server"] = "localhost";
        m_jsonObject["verb"] = 0;
        m_jsonObject["externalProcessTimeout"] = 3;
        m_jsonObject["headers"] = {"header1","header2"},
        m_jsonObject["maxJsonSize"] = 10;
        m_jsonObject["port"] = 123;
        m_jsonObject["resourceRoot"] = "TEST";

        m_jsonObject["messageRelays"] = {
            {{"authentication", 1}, {"certPath", "certpath"}, {"password", "relaypw"}, {"port", 456}, {"url", "relay"}, {"username", "relayuser"}}
        };

        m_jsonObject["proxies"] = {
            {{"authentication", 1}, {"password", "relaypw"}, {"port", 456}, {"url", "relay1"}, {"username", "relayuser"}}
        };
    }
};


TEST_F(TelemetryConfigTest, deserialiseStringToConfigAndBackToString) // NOLINT
{
    std::string originalJsonString = m_jsonObject.dump();
    std::cout << originalJsonString;

    // Convert the json string to a config object
    Config config = TelemetryConfigSerialiser::deserialise(originalJsonString);

    // Convert the config object to a json string
    std::string newJsonString = TelemetryConfigSerialiser::serialise(config);

    EXPECT_EQ(originalJsonString, newJsonString);
}

TEST_F(TelemetryConfigTest, serialiseDeserialise) // NOLINT
{
    // Convert the config object to a json string
    std::string jsonString = TelemetryConfigSerialiser::serialise(m_config);

    // Convert the json string to a config object, then back to a json string
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(m_config, newConfig);
}