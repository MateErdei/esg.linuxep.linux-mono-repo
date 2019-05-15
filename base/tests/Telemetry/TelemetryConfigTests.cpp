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
        m_config.m_verb = Common::HttpSenderImpl::RequestType::GET;
        m_config.m_externalProcessTimeout = 3;
        m_config.m_externalProcessRetries = 2;
        m_config.m_headers = { "header1", "header2" };
        m_config.m_maxJsonSize = 10;
        m_config.m_port = 123;
        m_config.m_resourceRoute = "TEST";

        messageRelay.m_url = "relay";
        messageRelay.m_port = 456;
        messageRelay.m_authentication = MessageRelay::Authentication::basic;
        messageRelay.m_id = "ID";
        messageRelay.m_priority = 2;
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
        m_jsonObject["externalProcessRetries"] = 2;
        m_jsonObject["headers"] = { "header1", "header2" }, m_jsonObject["maxJsonSize"] = 10;
        m_jsonObject["port"] = 123;
        m_jsonObject["resourceRoute"] = "TEST";

        m_jsonObject["messageRelays"] = { { { "authentication", 1 },
                                            { "id", "ID" },
                                            { "priority", 2 },
                                            { "password", "relaypw" },
                                            { "port", 456 },
                                            { "url", "relay" },
                                            { "username", "relayuser" } } };

        m_jsonObject["proxies"] = { { { "authentication", 1 },
                                      { "password", "relaypw" },
                                      { "port", 456 },
                                      { "url", "relay1" },
                                      { "username", "relayuser" } } };
    }
};

TEST_F(TelemetryConfigTest, deserialiseStringToConfigAndBackToString) // NOLINT
{
    std::string originalJsonString = m_jsonObject.dump();

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

TEST_F(TelemetryConfigTest, invalidConfigCannotBeSerialised) // NOLINT
{
    Config invalidConfig = m_config;
    invalidConfig.m_port = 65536;

    // Try to convert the invalidConfig object to a json string
    EXPECT_THROW(TelemetryConfigSerialiser::serialise(invalidConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, invalidJsonCannotBeDeserialised) // NOLINT
{
    nlohmann::json invalidJsonObject = m_jsonObject;
    invalidJsonObject["port"] = 65536;

    std::string invalidJsonString = invalidJsonObject.dump();

    // Try to convert the invalidJsonString to a config object
    EXPECT_THROW(TelemetryConfigSerialiser::deserialise(invalidJsonString), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, brokenJsonCannotBeDeserialised) // NOLINT
{
    // Try to convert broken JSON to a config object
    EXPECT_THROW(TelemetryConfigSerialiser::deserialise("imbroken:("), nlohmann::detail::parse_error); // NOLINT
}

TEST_F(TelemetryConfigTest, UnauthenticatedProxyWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.m_proxies.clear();
    Proxy customProxy;

    customProxy.m_url = "proxy";
    customProxy.m_port = 789;
    customProxy.m_authentication = Proxy::Authentication::none;

    customConfig.m_proxies.push_back(customProxy);

    std::string jsonString = TelemetryConfigSerialiser::serialise(customConfig);
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(TelemetryConfigTest, UnauthenticatedProxyWithCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.m_proxies.clear();
    Proxy customProxy;

    customProxy.m_url = "proxy";
    customProxy.m_port = 789;
    customProxy.m_authentication = Proxy::Authentication::none;
    customProxy.m_username = "proxyuser";
    customProxy.m_password = "proxypw";

    customConfig.m_proxies.push_back(customProxy);

    EXPECT_THROW(TelemetryConfigSerialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, AuthenticatedProxyWithCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.m_proxies.clear();
    Proxy customProxy;

    customProxy.m_url = "proxy";
    customProxy.m_port = 789;
    customProxy.m_authentication = Proxy::Authentication::digest;
    customProxy.m_username = "proxyuser";
    customProxy.m_password = "proxypw";

    customConfig.m_proxies.push_back(customProxy);

    std::string jsonString = TelemetryConfigSerialiser::serialise(customConfig);
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(TelemetryConfigTest, AuthenticatedProxyWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.m_proxies.clear();
    Proxy customProxy;

    customProxy.m_url = "proxy";
    customProxy.m_port = 789;
    customProxy.m_authentication = Proxy::Authentication::digest;

    customConfig.m_proxies.push_back(customProxy);

    EXPECT_THROW(TelemetryConfigSerialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, UnauthenticatedMessageRelayWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.m_messageRelays.clear();
    MessageRelay messageRelay;

    messageRelay.m_url = "proxy";
    messageRelay.m_port = 789;
    messageRelay.m_authentication = MessageRelay::Authentication::none;

    customConfig.m_messageRelays.push_back(messageRelay);

    std::string jsonString = TelemetryConfigSerialiser::serialise(customConfig);
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(TelemetryConfigTest, UnauthenticatedMessageRelayWithCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.m_messageRelays.clear();
    MessageRelay messageRelay;

    messageRelay.m_url = "proxy";
    messageRelay.m_port = 789;
    messageRelay.m_authentication = MessageRelay::Authentication::none;
    messageRelay.m_username = "proxyuser";
    messageRelay.m_password = "proxypw";

    customConfig.m_messageRelays.push_back(messageRelay);

    EXPECT_THROW(TelemetryConfigSerialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, AuthenticatedMessageRelayWithCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.m_messageRelays.clear();
    MessageRelay messageRelay;

    messageRelay.m_url = "proxy";
    messageRelay.m_port = 789;
    messageRelay.m_authentication = MessageRelay::Authentication::digest;
    messageRelay.m_username = "proxyuser";
    messageRelay.m_password = "proxypw";

    customConfig.m_messageRelays.push_back(messageRelay);

    std::string jsonString = TelemetryConfigSerialiser::serialise(customConfig);
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(TelemetryConfigTest, AuthenticatedMessageRelayWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.m_messageRelays.clear();
    MessageRelay messageRelay;

    messageRelay.m_url = "proxy";
    messageRelay.m_port = 789;
    messageRelay.m_authentication = MessageRelay::Authentication::digest;

    customConfig.m_messageRelays.push_back(messageRelay);

    EXPECT_THROW(TelemetryConfigSerialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST(MessageRelayTest, messageRelayEquality) // NOLINT
{
    MessageRelay a;
    MessageRelay b;
    a.m_port = 2;
    ASSERT_NE(a, b);

    b.m_port = 2;
    ASSERT_EQ(a, b);
}

TEST(ProxyTest, proxyEquality) // NOLINT
{
    Proxy a;
    Proxy b;
    a.m_port = 2;
    ASSERT_NE(a, b);

    b.m_port = 2;
    ASSERT_EQ(a, b);
}

TEST(ConfigTest, configEquality) // NOLINT
{
    Config a;
    Config b;

    a.m_server = "server";
    ASSERT_NE(a, b);

    b.m_server = "server";
    ASSERT_EQ(a, b);
}
