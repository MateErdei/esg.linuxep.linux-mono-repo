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
    Config config;
    MessageRelay messageRelay;
    Proxy proxy;

    // nlohmann::json j;

    void SetUp() override
    {
        config.m_server = "localhost";
        config.m_verb  = Config::HttpVerb::GET;
        config.m_externalProcessTimeout = 3;
        config.m_headers = {"header1", "header2"};
        config.m_maxJsonSize = 10;
        config.m_port = 123;
        config.m_resourceRoute = "TEST";

        messageRelay.m_url = "relay";
        messageRelay.m_port = 456;
        messageRelay.m_authentication = MessageRelay::Authentication::basic;
        messageRelay.m_certificatePath = "certpath";
        messageRelay.m_username = "relayuser";
        messageRelay.m_password = "relaypw";

        config.m_messageRelays.push_back(messageRelay);

        proxy.m_url = "proxy";
        proxy.m_port = 789;
        proxy.m_authentication = MessageRelay::Authentication::basic;
        proxy.m_username = "proxyuser";
        proxy.m_password = "proxypw";

        config.m_proxies.push_back(proxy);

//        j["server"] = "localhost";
//        j["verb"] = 0;
//        j["externalProcessTimeout"] = 3;
//        j["headers"] = {"header1","header2"},
//            j["maxJsonSize"] = 10;
//        j["port"] = 123;
//        j["resourceRoute"] = "TEST";
//
//        j["messageRelays"] = { {"authentication", 1}, {"certPath", "certpath"}, {"password", "relaypw"}, {"port", 456}, {"url", "relay"}, {"username", "relayuser"} };
//
//        j["proxies"] = { {"authentication", 1}, {"password", "relaypw"}, {"port", 456}, {"url", "relay"}, {"username", "relayuser"} };
    }
};

TEST_F(TelemetryConfigTest, serialiseTelemetryConfig) // NOLINT
{
    TelemetryConfigSerialiser serialiser;
    std::cout << serialiser.serialise(config) << std::endl;
}

TEST_F(TelemetryConfigTest, deserialiseTelemetryConfig) // NOLINT
{
    TelemetryConfigSerialiser serialiser;

    // Convert the config object to a json string
    std::string jsonString = serialiser.serialise(config);
    std::cout << jsonString << std::endl;

    // Convert the json string to a config object, then back to a json string
    Config newConfig = serialiser.deserialise(jsonString);

    EXPECT_EQ(config, newConfig);
}