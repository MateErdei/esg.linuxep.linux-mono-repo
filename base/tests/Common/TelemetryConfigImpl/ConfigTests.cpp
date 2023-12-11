// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Common/TelemetryConfigImpl/Config.h"
#include "Common/TelemetryConfigImpl/Serialiser.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

using ::testing::StrictMock;
using namespace Common::TelemetryConfigImpl;

class ConfigTests : public ::testing::Test
{
public:
    Config m_config;

    nlohmann::json m_jsonObject;

    const unsigned int m_validPort = 300;
    const unsigned int m_invalidPort = 70000;
    const std::string m_jsonString =
        R"({"additionalHeaders":{"header1":"value1","header2":"value2"},"externalProcessWaitRetries":2,"externalProcessWaitTime":3,)"
        R"("interval":42,"maxJsonSize":10,)"
        R"("messageRelays":[{"authentication":1,"id":"ID",)"
        R"("password":"CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=",)"
        R"("port":300,"priority":2,"url":"relay","username":"relayuser"}],)"
        R"("pluginConnectionTimeout":5000,"pluginSendReceiveTimeout":5000,"port":300,)"
        R"("proxies":[{"authentication":1,)"
        R"("password":"CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=","port":300,"url":"proxy",)"
        R"("username":"proxyuser"}],"resourcePath":"test/id.json","resourceRoot":"test","server":"localhost",)"
        R"("telemetryServerCertificatePath":"some/path","verb":"GET"})";

    void SetUp() override
    {
        MessageRelay messageRelay;
        Proxy proxy;

        m_config.setServer("localhost");
        m_config.setVerb("GET");
        m_config.setInterval(42);
        m_config.setExternalProcessWaitTime(3);
        m_config.setExternalProcessWaitRetries(2);

        Common::HttpRequests::Headers headers;
        headers.insert({"header1", "value1"});
        headers.insert({"header2", "value2"});
        m_config.setHeaders(headers);

        m_config.setMaxJsonSize(10);
        m_config.setPort(m_validPort);
        m_config.setResourceRoot("test");
        m_config.setResourcePath("test/id.json");
        m_config.setTelemetryServerCertificatePath("some/path");

        messageRelay.setUrl("relay");
        messageRelay.setPort(m_validPort);
        messageRelay.setAuthentication(MessageRelay::Authentication::basic);
        messageRelay.setId("ID");
        messageRelay.setPriority(2);
        messageRelay.setUsername("relayuser");
        messageRelay.setPassword("CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");

        m_config.setMessageRelays({ messageRelay });

        proxy.setUrl("proxy");
        proxy.setPort(m_validPort);
        proxy.setAuthentication(MessageRelay::Authentication::basic);
        proxy.setUsername("proxyuser");
        proxy.setPassword("CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");

        m_config.setProxies({ proxy });

        m_jsonObject["server"] = "localhost";
        m_jsonObject["verb"] = "PUT";
        m_jsonObject["externalProcessWaitTime"] = 3;
        m_jsonObject["externalProcessWaitRetries"] = 2;
        m_jsonObject["additionalHeaders"] = headers,
        m_jsonObject["maxJsonSize"] = 10;
        m_jsonObject["port"] = m_validPort;
        m_jsonObject["resourceRoot"] = "TEST";
        m_jsonObject["telemetryServerCertificatePath"] = "some/path";

        m_jsonObject["messageRelays"] = { { { "authentication", 1 },
                                            { "id", "ID" },
                                            { "priority", 2 },
                                            { "password",
                                              "CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=" },
                                            { "port", m_validPort },
                                            { "url", "relay" },
                                            { "username", "relayuser" } } };

        m_jsonObject["proxies"] = { { { "authentication", 1 },
                                      { "password",
                                        "CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=" },
                                      { "port", m_validPort },
                                      { "url", "relay1" },
                                      { "username", "relayuser" } } };
    }
};

TEST_F(ConfigTests, defaultConstrutor)
{
    Config c;

    EXPECT_EQ(DEFAULT_MAX_JSON_SIZE, c.getMaxJsonSize());
    EXPECT_EQ(DEFAULT_PROCESS_WAIT_RETRIES, c.getExternalProcessWaitRetries());
    EXPECT_EQ(DEFAULT_PROCESS_WAIT_TIME, c.getExternalProcessWaitTime());
    EXPECT_EQ(VERB_PUT, c.getVerb());
    EXPECT_EQ(DEFAULT_PLUGIN_SEND_RECEIVE_TIMEOUT, c.getPluginSendReceiveTimeout());
    EXPECT_EQ(DEFAULT_PLUGIN_CONNECTION_TIMEOUT, c.getPluginConnectionTimeout());

    ASSERT_TRUE(c.isValid());
}

TEST_F(ConfigTests, serialise)
{
    std::string jsonString = Serialiser::serialise(m_config);
    EXPECT_EQ(m_jsonString, jsonString);
}

TEST_F(ConfigTests, deserialise)
{
    Config c = Serialiser::deserialise(m_jsonString);
    EXPECT_EQ(m_config, c);
}

TEST_F(ConfigTests, serialiseAndDeserialise)
{
    ASSERT_EQ(m_config, Serialiser::deserialise(Serialiser::serialise(m_config)));
}

TEST_F(ConfigTests, invalidConfigCannotBeSerialised)
{
    Config invalidConfig = m_config;
    invalidConfig.setPort(m_invalidPort);

    // Try to convert the invalidConfig object to a json string
    EXPECT_THROW(Serialiser::serialise(invalidConfig), std::invalid_argument);
}

TEST_F(ConfigTests, invalidJsonCannotBeDeserialised)
{
    nlohmann::json invalidJsonObject = m_jsonObject;
    invalidJsonObject["port"] = m_invalidPort;

    std::string invalidJsonString = invalidJsonObject.dump();

    // Try to convert the invalidJsonString to a config object
    EXPECT_THROW(Serialiser::deserialise(invalidJsonString), std::runtime_error);
}

TEST_F(ConfigTests, brokenJsonCannotBeDeserialised)
{
    // Try to convert broken JSON to a config object
    EXPECT_THROW(Serialiser::deserialise("imbroken:("), std::runtime_error);
}

TEST_F(ConfigTests, parseValidConfigJsonDirectlySucceeds)
{
    const std::string validTelemetryJson = R"(
    {
        "telemetryServerCertificatePath": "",
        "externalProcessWaitRetries": 10,
        "externalProcessWaitTime": 100,
        "additionalHeaders": {"x-amz-acl":"bucket-owner-full-control"},
        "maxJsonSize": 100000,
        "messageRelays": [],
        "port": 443,
        "proxies": [],
        "resourceRoot": "linux/dev",
        "server": "localhost",
        "verb": "PUT",
        "pluginSendReceiveTimeout": 1000,
        "pluginConnectionTimeout": 1000
    })";

    Serialiser::deserialise(validTelemetryJson);
}

TEST_F(ConfigTests, parseSubsetConfigJsonDirectlySucceeds)
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

TEST_F(ConfigTests, parseSupersetConfigJsonDirectlySucceeds)
{
    const std::string supersetTelemetryJson = R"(
    {
        "telemetryServerCertificatePath": "",
        "externalProcessWaitRetries": 10,
        "externalProcessWaitTime": 100,
        "additionalHeaders": {"x-amz-acl":"bucket-owner-full-control"},
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

TEST_F(ConfigTests, UnauthenticatedProxyWithoutCredentials)
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::none);

    customConfig.setProxies({ customProxy });

    std::string jsonString = Serialiser::serialise(customConfig);
    Config newConfig = Serialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(ConfigTests, UnauthenticatedProxyWithCredentials)
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::none);
    customProxy.setUsername("proxyuser");
    customProxy.setPassword("proxypw");

    customConfig.setProxies({ customProxy });

    EXPECT_THROW(Serialiser::serialise(customConfig), std::invalid_argument);
}

TEST_F(ConfigTests, AuthenticatedProxyWithCredentials)
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::digest);
    customProxy.setUsername("proxyuser");
    customProxy.setPassword("proxypw");

    customConfig.setProxies({ customProxy });

    std::string jsonString = Serialiser::serialise(customConfig);
    Config newConfig = Serialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(ConfigTests, AuthenticatedProxyWithoutCredentials)
{
    Config customConfig = m_config;
    customConfig.setProxies({});
    Proxy customProxy;

    customProxy.setUrl("proxy");
    customProxy.setPort(m_validPort);
    customProxy.setAuthentication(Proxy::Authentication::digest);

    customConfig.setProxies({ customProxy });

    EXPECT_THROW(Serialiser::serialise(customConfig), std::invalid_argument);
}

TEST_F(ConfigTests, UnauthenticatedMessageRelayWithoutCredentials)
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::none);

    customConfig.setMessageRelays({ messageRelay });

    std::string jsonString = Serialiser::serialise(customConfig);
    Config newConfig = Serialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(ConfigTests, UnauthenticatedMessageRelayWithCredentials)
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::none);
    messageRelay.setUsername("proxyuser");
    messageRelay.setPassword("proxypw");

    customConfig.setMessageRelays({ messageRelay });

    EXPECT_THROW(Serialiser::serialise(customConfig), std::invalid_argument);
}

TEST_F(ConfigTests, AuthenticatedMessageRelayWithCredentials)
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::digest);
    messageRelay.setUsername("proxyuser");
    messageRelay.setPassword("proxypw");

    customConfig.setMessageRelays({ messageRelay });

    std::string jsonString = Serialiser::serialise(customConfig);
    Config newConfig = Serialiser::deserialise(jsonString);

    EXPECT_EQ(customConfig, newConfig);
}

TEST_F(ConfigTests, AuthenticatedMessageRelayWithoutCredentials)
{
    Config customConfig = m_config;
    customConfig.setMessageRelays({});
    MessageRelay messageRelay;

    messageRelay.setUrl("proxy");
    messageRelay.setPort(m_validPort);
    messageRelay.setAuthentication(MessageRelay::Authentication::digest);

    customConfig.setMessageRelays({ messageRelay });

    EXPECT_THROW(Serialiser::serialise(customConfig), std::invalid_argument);
}

TEST_F(ConfigTests, InvalidPort)
{
    Config c = m_config;
    c.setPort(m_invalidPort);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument);
}

TEST_F(ConfigTests, ProcessWaitTimeoutTooSmall)
{
    Config c = m_config;
    c.setExternalProcessWaitTime(MIN_PROCESS_WAIT_TIMEOUT - 1);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument);
}

TEST_F(ConfigTests, ProcessWaitTimeoutTooLarge)
{
    Config c = m_config;
    c.setExternalProcessWaitTime(MAX_PROCESS_WAIT_TIMEOUT + 1);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument);
}

TEST_F(ConfigTests, ProcessWaitRetriesTooLarge)
{
    Config c = m_config;
    c.setExternalProcessWaitRetries(MAX_PROCESS_WAIT_RETRIES + 1);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument);
}

TEST_F(ConfigTests, PluginSendReceiveTimeoutTooLow)
{
    Config c = m_config;
    c.setPluginSendReceiveTimeout(MIN_PLUGIN_TIMEOUT - 1);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument);
}

TEST_F(ConfigTests, PluginSendReceiveTimeoutTooLarge)
{
    Config c = m_config;
    c.setPluginSendReceiveTimeout(MAX_PLUGIN_TIMEOUT + 1);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument);
}

TEST_F(ConfigTests, PluginConnectionTimeoutTooLow)
{
    Config c = m_config;
    c.setPluginConnectionTimeout(MIN_PLUGIN_TIMEOUT - 1);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument);
}

TEST_F(ConfigTests, PluginConnectionTimeoutTooLarge)
{
    Config c = m_config;
    c.setPluginConnectionTimeout(MAX_PLUGIN_TIMEOUT + 1);
    EXPECT_THROW(Serialiser::serialise(c), std::invalid_argument);
}

TEST_F(ConfigTests, configEquality)
{
    Config a;
    Config b;

    a.setServer("server");
    ASSERT_NE(a, b);

    b.setServer("server");
    ASSERT_EQ(a, b);
}

TEST_F(ConfigTests, buildExeConfigFromSupplementaryConfig)
{
    const std::string resourceName = "name";
    Config exeConfig = Config::buildExeConfigFromTelemetryConfig(m_config, "telemetryHost", resourceName);
    EXPECT_EQ("telemetryHost", exeConfig.getServer());
    EXPECT_EQ(m_config.getPort(), exeConfig.getPort());
    EXPECT_EQ(m_config.getVerb(), exeConfig.getVerb());
    EXPECT_EQ(m_config.getHeaders(), exeConfig.getHeaders());
    EXPECT_EQ(m_config.getResourceRoot() + "/" + resourceName, exeConfig.getResourcePath());

    EXPECT_EQ("", exeConfig.getResourceRoot());
    EXPECT_NE(m_config.getInterval(), exeConfig.getInterval());
}
