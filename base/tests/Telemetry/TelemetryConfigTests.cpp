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
    const std::string m_jsonString = R"({"externalProcessRetries":2,"externalProcessTimeout":3,"headers":["header1","header2"],"maxJsonSize":10,"messageRelays":[{"authentication":1,"id":"ID","password":"CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=","port":300,"priority":2,"url":"relay","username":"relayuser"}],"port":300,"proxies":[{"authentication":1,"password":"CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=","port":300,"url":"proxy","username":"proxyuser"}],"resourceRoute":"TEST","server":"localhost","telemetryServerCertificatePath":"some/path","verb":"GET"})";

    void SetUp() override
    {
        MessageRelay messageRelay;
        Proxy proxy;

        m_config.setServer("localhost");
        m_config.setVerb("GET");
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
        messageRelay.setPassword("CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");

        m_config.setMessageRelays({messageRelay});

        proxy.setUrl("proxy");
        proxy.setPort(m_validPort);
        proxy.setAuthentication(MessageRelay::Authentication::basic);
        proxy.setUsername("proxyuser");
        proxy.setPassword("CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");

        m_config.setProxies({proxy});

        m_jsonObject["server"] = "localhost";
        m_jsonObject["verb"] = "PUT";
        m_jsonObject["externalProcessTimeout"] = 3;
        m_jsonObject["externalProcessRetries"] = 2;
        m_jsonObject["headers"] = { "header1", "header2" }, m_jsonObject["maxJsonSize"] = 10;
        m_jsonObject["port"] = m_validPort;
        m_jsonObject["resourceRoute"] = "TEST";
        m_jsonObject["telemetryServerCertificatePath"] = "some/path";

        m_jsonObject["messageRelays"] = { { { "authentication", 1 },
                                            { "id", "ID" },
                                            { "priority", 2 },
                                            { "password", "CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=" },
                                            { "port", m_validPort },
                                            { "url", "relay" },
                                            { "username", "relayuser" } } };

        m_jsonObject["proxies"] = { { { "authentication", 1 },
                                      { "password", "CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=" },
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
    EXPECT_EQ(VERB_PUT, c.getVerb());

    ASSERT_TRUE(c.isValid());
}

TEST_F(TelemetryConfigTest, serialise) // NOLINT
{
    std::string jsonString = TelemetryConfigSerialiser::serialise(m_config);
    EXPECT_EQ(m_jsonString, jsonString);
}

TEST_F(TelemetryConfigTest, deserialise) // NOLINT
{
    Config c = TelemetryConfigSerialiser::deserialise(m_jsonString);
    EXPECT_EQ(m_config, c);
}

TEST_F(TelemetryConfigTest, serialiseAndDeserialise) // NOLINT
{
    ASSERT_EQ(m_config, TelemetryConfigSerialiser::deserialise(TelemetryConfigSerialiser::serialise(m_config)));
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
