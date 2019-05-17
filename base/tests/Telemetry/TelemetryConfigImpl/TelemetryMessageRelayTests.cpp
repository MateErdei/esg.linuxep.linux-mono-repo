/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <Telemetry/TelemetryConfigImpl/Config.h>
#include <Telemetry/TelemetryConfigImpl/TelemetryConfigSerialiser.h>

using ::testing::StrictMock;
using namespace Telemetry::TelemetryConfigImpl;

class TelemetryMessageRelayTest : public ::testing::Test
{
public:
    MessageRelay m_messageRelay;

    void SetUp() override
    {
        m_messageRelay.setUrl("relay");
        m_messageRelay.setPort(456);
        m_messageRelay.setAuthentication(MessageRelay::Authentication::basic);
        m_messageRelay.setId("ID");
        m_messageRelay.setPriority(2);
        m_messageRelay.setUsername("relayuser");
        m_messageRelay.setPassword("relaypw");
    }
};

TEST_F(TelemetryMessageRelayTest, proxyEqualitySameObject) // NOLINT
{
    ASSERT_EQ(m_messageRelay, m_messageRelay);
}

TEST_F(TelemetryMessageRelayTest, proxyEqualityDiffObject) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    ASSERT_EQ(a, b);
}

TEST_F(TelemetryMessageRelayTest, proxyNotEqualPort) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setPort(1);
    ASSERT_NE(a, b);
}

TEST_F(TelemetryMessageRelayTest, proxyNotEqualUrl) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setUrl("blah");
    ASSERT_NE(a, b);
}

TEST_F(TelemetryMessageRelayTest, proxyNotEqualAuth) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setAuthentication(Proxy::Authentication::none);
    ASSERT_NE(a, b);
}

TEST_F(TelemetryMessageRelayTest, proxyNotEqualUsername) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setUsername("blah");
    ASSERT_NE(a, b);
}

TEST_F(TelemetryMessageRelayTest, proxyNotEqualPassword) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setPassword("blah");
    ASSERT_NE(a, b);
}

TEST_F(TelemetryMessageRelayTest, proxyValid) // NOLINT
{
    ASSERT_TRUE(m_messageRelay.isValidProxy());
}

TEST_F(TelemetryMessageRelayTest, proxyInvalidPort) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setPort(5000000);
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryMessageRelayTest, proxyInvalidAuthNoneWithUsername) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("should not be set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryMessageRelayTest, proxyInvalidAuthNoneWithPassword) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("should not be set");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryMessageRelayTest, proxyInvalidAuthBasicWithoutUsername) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryMessageRelayTest, proxyInvalidAuthBasicWithoutPassword) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryMessageRelayTest, proxyInvalidAuthDigestWithoutUsername) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryMessageRelayTest, proxyInvalidAuthDigestWithoutPassword) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST(MessageRelayTest, messageRelayEquality) // NOLINT
{
    MessageRelay a;
    MessageRelay b;
    a.setPort(2);
    ASSERT_NE(a, b);

    b.setPort(2);
    ASSERT_EQ(a, b);
}
