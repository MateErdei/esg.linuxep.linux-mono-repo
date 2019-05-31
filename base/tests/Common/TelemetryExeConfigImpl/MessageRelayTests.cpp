/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TelemetryExeConfigImpl/Config.h>
#include <Common/TelemetryExeConfigImpl/Serialiser.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::StrictMock;
using namespace Common::TelemetryExeConfigImpl;

class MessageRelayTests : public ::testing::Test
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

TEST_F(MessageRelayTests, proxyEqualitySameObject) // NOLINT
{
    ASSERT_EQ(m_messageRelay, m_messageRelay);
}

TEST_F(MessageRelayTests, proxyEqualityDiffObject) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    ASSERT_EQ(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualPort) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setPort(1);
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualUrl) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setUrl("blah");
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualAuth) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setAuthentication(Proxy::Authentication::none);
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualUsername) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setUsername("blah");
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualPassword) // NOLINT
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setPassword("blah");
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyValid) // NOLINT
{
    ASSERT_TRUE(m_messageRelay.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidPort) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setPort(5000000);
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthNoneWithUsername) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("should not be set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthNoneWithPassword) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("should not be set");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthBasicWithoutUsername) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthBasicWithoutPassword) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthDigestWithoutUsername) // NOLINT
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthDigestWithoutPassword) // NOLINT
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
