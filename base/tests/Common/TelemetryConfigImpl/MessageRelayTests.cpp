// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Common/TelemetryConfigImpl/Config.h"
#include "Common/TelemetryConfigImpl/Serialiser.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::StrictMock;
using namespace Common::TelemetryConfigImpl;

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

TEST_F(MessageRelayTests, proxyEqualitySameObject)
{
    ASSERT_EQ(m_messageRelay, m_messageRelay);
}

TEST_F(MessageRelayTests, proxyEqualityDiffObject)
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    ASSERT_EQ(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualPort)
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setPort(1);
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualUrl)
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setUrl("blah");
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualAuth)
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setAuthentication(Proxy::Authentication::none);
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualUsername)
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setUsername("blah");
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyNotEqualPassword)
{
    MessageRelay a = m_messageRelay;
    MessageRelay b = m_messageRelay;
    b.setPassword("blah");
    ASSERT_NE(a, b);
}

TEST_F(MessageRelayTests, proxyValid)
{
    ASSERT_TRUE(m_messageRelay.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidPort)
{
    MessageRelay p = m_messageRelay;
    p.setPort(5000000);
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthNoneWithUsername)
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("should not be set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthNoneWithPassword)
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("should not be set");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthBasicWithoutUsername)
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthBasicWithoutPassword)
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthDigestWithoutUsername)
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(MessageRelayTests, proxyInvalidAuthDigestWithoutPassword)
{
    MessageRelay p = m_messageRelay;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST(MessageRelayTest, messageRelayEquality)
{
    MessageRelay a;
    MessageRelay b;
    a.setPort(2);
    ASSERT_NE(a, b);

    b.setPort(2);
    ASSERT_EQ(a, b);
}
