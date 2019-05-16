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

    const unsigned int m_validPort = 300;
    const unsigned int m_invalidPort = 70000;

    void SetUp() override
    {
        MessageRelay messageRelay;
        Proxy proxy;

        m_config.setServer("localhost");
        m_config.setVerb(Common::HttpSenderImpl::RequestType::GET);
        m_config.setExternalProcessTimeout(3);
        m_config.setExternalProcessRetries(2);
        m_config.setHeaders({ "header1", "header2" });
        m_config.setMaxJsonSize(10);
        m_config.setPort(m_validPort);
        m_config.setResourceRoute("TEST");
        m_config.setTelemetryServerCertificatePath("some/path");

        messageRelay.setUrl("relay");
        messageRelay.setPort(m_validPort);
        messageRelay.setAuthentication(MessageRelay::Authentication::basic);
        messageRelay.setId("ID");
        messageRelay.setPriority(2);
        messageRelay.setUsername("relayuser");
        messageRelay.setPassword("relaypw");

        m_config.setMessageRelays({messageRelay});

        proxy.setUrl("proxy");
        proxy.setPort(m_validPort);
        proxy.setAuthentication(MessageRelay::Authentication::basic);
        proxy.setUsername("proxyuser");
        proxy.setPassword("proxypw");

        m_config.setProxies({proxy});

        m_jsonObject["server"] = "localhost";
        m_jsonObject["verb"] = 0;
        m_jsonObject["externalProcessTimeout"] = 3;
        m_jsonObject["externalProcessRetries"] = 2;
        m_jsonObject["headers"] = { "header1", "header2" }, m_jsonObject["maxJsonSize"] = 10;
        m_jsonObject["port"] = m_validPort;
        m_jsonObject["resourceRoute"] = "TEST";
        m_jsonObject["telemetryServerCertificatePath"] = "some/path";

        m_jsonObject["messageRelays"] = { { { "authentication", 1 },
                                            { "id", "ID" },
                                            { "priority", 2 },
                                            { "password", "relaypw" },
                                            { "port", m_validPort },
                                            { "url", "relay" },
                                            { "username", "relayuser" } } };

        m_jsonObject["proxies"] = { { { "authentication", 1 },
                                      { "password", "relaypw" },
                                      { "port", m_validPort },
                                      { "url", "relay1" },
                                      { "username", "relayuser" } } };
    }
};

TEST_F(TelemetryConfigTest, defaultConstrutor) // NOLINT
{
    Config c;

    EXPECT_EQ(DEFAULT_MAX_JSON_SIZE, c.getMaxJsonSize());
    EXPECT_EQ(DEFAULT_RETRIES, c.getExternalProcessRetries());
    EXPECT_EQ(DEFAULT_TIMEOUT, c.getExternalProcessTimeout());
}

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
    invalidConfig.setPort(m_invalidPort);

    // Try to convert the invalidConfig object to a json string
    EXPECT_THROW(TelemetryConfigSerialiser::serialise(invalidConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, invalidJsonCannotBeDeserialised) // NOLINT
{
    nlohmann::json invalidJsonObject = m_jsonObject;
    invalidJsonObject["port"] = m_invalidPort;

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
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::none);

    customConfig.setProxies({customProxy});

    std::string jsonString = TelemetryConfigSerialiser::serialise(customConfig);
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(TelemetryConfigTest, UnauthenticatedProxyWithCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::none);
    customProxy.setUsername("proxyuser");
    customProxy.setPassword("proxypw");

    customConfig.setProxies({customProxy});

    EXPECT_THROW(TelemetryConfigSerialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, AuthenticatedProxyWithCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::digest);
    customProxy.setUsername("proxyuser");
    customProxy.setPassword("proxypw");

    customConfig.setProxies({customProxy});

    std::string jsonString = TelemetryConfigSerialiser::serialise(customConfig);
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(TelemetryConfigTest, AuthenticatedProxyWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::digest);

    customConfig.setProxies({customProxy});

    EXPECT_THROW(TelemetryConfigSerialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, UnauthenticatedMessageRelayWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::none);

    customConfig.setMessageRelays({messageRelay});

    std::string jsonString = TelemetryConfigSerialiser::serialise(customConfig);
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(TelemetryConfigTest, UnauthenticatedMessageRelayWithCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::none);
    messageRelay.setUsername("proxyuser");
    messageRelay.setPassword("proxypw");

    customConfig.setMessageRelays({messageRelay});

    EXPECT_THROW(TelemetryConfigSerialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, AuthenticatedMessageRelayWithCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::digest);
    messageRelay.setUsername("proxyuser");
    messageRelay.setPassword("proxypw");

    customConfig.setMessageRelays({messageRelay});

    std::string jsonString = TelemetryConfigSerialiser::serialise(customConfig);
    Config newConfig = TelemetryConfigSerialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(TelemetryConfigTest, AuthenticatedMessageRelayWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::digest);

    customConfig.setMessageRelays({messageRelay});

    EXPECT_THROW(TelemetryConfigSerialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, InvalidPort) // NOLINT
{
    Config c = m_config;
    c.setPort(m_invalidPort);
    EXPECT_THROW(TelemetryConfigSerialiser::serialise(c), std::invalid_argument); // NOLINT
}

TEST_F(TelemetryConfigTest, InvalidTimeout) // NOLINT
{
    Config c = m_config;
    c.setExternalProcessTimeout(0);
    EXPECT_THROW(TelemetryConfigSerialiser::serialise(c), std::invalid_argument); // NOLINT
}

TEST(ConfigTest, configEquality) // NOLINT
{
    Config a;
    Config b;

    a.setServer("server");
    ASSERT_NE(a, b);

    b.setServer("server");
    ASSERT_EQ(a, b);
}
