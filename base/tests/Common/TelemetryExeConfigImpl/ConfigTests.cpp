/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <Common/TelemetryExeConfigImpl/Config.h>
#include <Common/TelemetryExeConfigImpl/Serialiser.h>
#include <json.hpp>

using ::testing::StrictMock;
using namespace Common::TelemetryExeConfigImpl;

class ConfigTests : public ::testing::Test
{
public:
    Config m_config;

    nlohmann::json m_jsonObject;

    const unsigned int m_validPort = 300;
    const unsigned int m_invalidPort = 70000;
    const std::string m_jsonString = R"({"additionalHeaders":["header1","header2"],"externalProcessWaitRetries":2,"externalProcessWaitTime":3,"maxJsonSize":10,"messageRelays":[{"authentication":1,"id":"ID","password":"CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=","port":300,"priority":2,"url":"relay","username":"relayuser"}],"port":300,"proxies":[{"authentication":1,"password":"CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=","port":300,"url":"proxy","username":"proxyuser"}],"resourceRoot":"TEST","server":"localhost","telemetryServerCertificatePath":"some/path","verb":"GET"})";

    void SetUp() override
    {
        MessageRelay messageRelay;
        Proxy proxy;

        m_config.setServer("localhost");
        m_config.setVerb("GET");
        m_config.setExternalProcessWaitTime(3);
        m_config.setExternalProcessWaitRetries(2);
        m_config.setHeaders({ "header1", "header2" });
        m_config.setMaxJsonSize(10);
        m_config.setPort(m_validPort);
        m_config.setResourceRoot("TEST");
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
        m_jsonObject["externalProcessWaitTime"] = 3;
        m_jsonObject["externalProcessWaitRetries"] = 2;
        m_jsonObject["additionalHeaders"] = { "header1", "header2" }, m_jsonObject["maxJsonSize"] = 10;
        m_jsonObject["port"] = m_validPort;
        m_jsonObject["resourceRoot"] = "TEST";
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

TEST_F(ConfigTests, defaultConstrutor) // NOLINT
{
    Config c;

    EXPECT_EQ(DEFAULT_MAX_JSON_SIZE, c.getMaxJsonSize());
    EXPECT_EQ(DEFAULT_PROCESS_WAIT_RETRIES, c.getExternalProcessWaitRetries());
    EXPECT_EQ(DEFAULT_PROCESS_WAIT_TIME, c.getExternalProcessWaitTime());
    EXPECT_EQ(VERB_PUT, c.getVerb());

    ASSERT_TRUE(c.isValid());
}

TEST_F(ConfigTests, serialise) // NOLINT
{
    std::string jsonString = Serialiser::serialise(m_config);
    EXPECT_EQ(m_jsonString, jsonString);
}

TEST_F(ConfigTests, deserialise) // NOLINT
{
    Config c = Serialiser::deserialise(m_jsonString);
    EXPECT_EQ(m_config, c);
}

TEST_F(ConfigTests, serialiseAndDeserialise) // NOLINT
{
    ASSERT_EQ(m_config, Serialiser::deserialise(Serialiser::serialise(m_config)));
}

TEST_F(ConfigTests, invalidConfigCannotBeSerialised) // NOLINT
{
    Config invalidConfig = m_config;
    invalidConfig.setPort(m_invalidPort);

    // Try to convert the invalidConfig object to a json string
    EXPECT_THROW(Serialiser::serialise(invalidConfig), std::invalid_argument); // NOLINT
}

TEST_F(ConfigTests, invalidJsonCannotBeDeserialised) // NOLINT
{
    nlohmann::json invalidJsonObject = m_jsonObject;
    invalidJsonObject["port"] = m_invalidPort;

    std::string invalidJsonString = invalidJsonObject.dump();

    // Try to convert the invalidJsonString to a config object
    EXPECT_THROW(Serialiser::deserialise(invalidJsonString), std::runtime_error); // NOLINT
}

TEST_F(ConfigTests, brokenJsonCannotBeDeserialised) // NOLINT
{
    // Try to convert broken JSON to a config object
    EXPECT_THROW(Serialiser::deserialise("imbroken:("), std::runtime_error); // NOLINT
}

TEST_F(ConfigTests, parseValidConfigJsonDirectlySucceeds) // NOLINT
{
    const std::string validTelemetryJson = R"(
    {
        "telemetryServerCertificatePath": "",
        "externalProcessWaitRetries": 10,
        "externalProcessWaitTime": 100,
        "additionalHeaders": ["x-amz-acl:bucket-owner-full-control"],
        "maxJsonSize": 100000,
        "messageRelays": [],
        "port": 443,
        "proxies": [],
        "resourceRoot": "linux/dev",
        "server": "localhost",
        "verb": "PUT"
    })";

    Serialiser::deserialise(validTelemetryJson);
}

TEST_F(ConfigTests, parseSubsetConfigJsonDirectlySucceeds) // NOLINT
{
    const std::string subsetTelemetryJson = R"(
    {
        "port": 443,
        "proxies": [],
        "resourceRoot": "linux/dev",
        "server": "localhost",
        "verb": "PUT"
    })";

    Serialiser::deserialise(subsetTelemetryJson);
}

TEST_F(ConfigTests, parseSupersetConfigJsonDirectlySucceeds) // NOLINT
{
    const std::string supersetTelemetryJson = R"(
    {
        "telemetryServerCertificatePath": "",
        "externalProcessWaitRetries": 10,
        "externalProcessWaitTime": 100,
        "additionalHeaders": ["x-amz-acl:bucket-owner-full-control"],
        "maxJsonSize": 100000,
        "messageRelays": [],
        "CURRENTLY_UNKNOWN" : "extra",
        "port": 443,
        "proxies": [],
        "resourceRoot": "linux/dev",
        "server": "localhost",
        "verb": "PUT"
    })";

    Serialiser::deserialise(supersetTelemetryJson);
}

TEST_F(ConfigTests, UnauthenticatedProxyWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::none);

    customConfig.setProxies({customProxy});

    std::string jsonString = Serialiser::serialise(customConfig);
    Config newConfig = Serialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(ConfigTests, UnauthenticatedProxyWithCredentials) // NOLINT
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

    EXPECT_THROW(Serialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(ConfigTests, AuthenticatedProxyWithCredentials) // NOLINT
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

    std::string jsonString = Serialiser::serialise(customConfig);
    Config newConfig = Serialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(ConfigTests, AuthenticatedProxyWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::digest);

    customConfig.setProxies({customProxy});

    EXPECT_THROW(Serialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(ConfigTests, UnauthenticatedMessageRelayWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::none);

    customConfig.setMessageRelays({messageRelay});

    std::string jsonString = Serialiser::serialise(customConfig);
    Config newConfig = Serialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(ConfigTests, UnauthenticatedMessageRelayWithCredentials) // NOLINT
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

    EXPECT_THROW(Serialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(ConfigTests, AuthenticatedMessageRelayWithCredentials) // NOLINT
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

    std::string jsonString = Serialiser::serialise(customConfig);
    Config newConfig = Serialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(ConfigTests, AuthenticatedMessageRelayWithoutCredentials) // NOLINT
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::digest);

    customConfig.setMessageRelays({messageRelay});

    EXPECT_THROW(Serialiser::serialise(customConfig), std::invalid_argument); // NOLINT
}

TEST_F(ConfigTests, InvalidPort) // NOLINT
{
    Config c = m_config;
    c.setPort(m_invalidPort);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument); // NOLINT
}

TEST_F(ConfigTests, InvalidTimeout) // NOLINT
{
    Config c = m_config;
    c.setExternalProcessWaitTime(0);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument); // NOLINT
}

TEST_F(ConfigTests, configEquality) // NOLINT
{
    Config a;
    Config b;

    a.setServer("server");
    ASSERT_NE(a, b);

    b.setServer("server");
    ASSERT_EQ(a, b);
}
